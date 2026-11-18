/*
 * ESP-NOW hub（ESP32-S3）—— 主机
 *
 * 1. 持续监听 ESP-NOW 广播。
 * 2. 收到从机事件后：串口打印来源，板载 LED 闪烁 3 秒。
 *    不同从机编号对应不同颜色，方便肉眼区分。
 * 3. 闪烁期间继续接收；新事件立即打印，并从最新事件重新计时。
 *
 * 后续这里会接 LCD + LVGL，把「串口打印 + 闪灯」替换成 UI 更新，
 * 事件队列和 hub_task 的结构可以原样保留。
 */

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "espnow_link.h"
#include "status_led.h"

static const char *TAG = "hub";

/* 主机最多同时区分的从机数量，与从机 Kconfig 的编号范围保持一致 */
#define HUB_MAX_NODES      8

#define BLINK_INTERVAL_US  (100 * 1000)        /* 每 100ms 翻转一次 */
#define BLINK_DURATION_US  (3 * 1000 * 1000)   /* 持续闪 3 秒 */
#define DEDUP_WINDOW_US    (1000 * 1000)       /* 1 秒内相同 seq 判定为重复 */

typedef struct {
    uint8_t device_id;
    uint16_t seq;
    uint8_t mac[6];
} hub_event_t;

static QueueHandle_t s_event_queue;

/* 去重状态：按从机编号保存上一次的 seq 和时间 */
static uint16_t s_last_seq[HUB_MAX_NODES];
static int64_t s_last_seq_time[HUB_MAX_NODES];

#ifdef CONFIG_HUB_LED_TYPE_WS2812
#define HUB_LED_TYPE STATUS_LED_WS2812
#else
#define HUB_LED_TYPE STATUS_LED_GPIO
#endif

#ifdef CONFIG_HUB_LED_ACTIVE_LOW
#define HUB_LED_ACTIVE_LOW true
#else
#define HUB_LED_ACTIVE_LOW false
#endif

/* 每个从机一种颜色；编号超出调色板时循环取用 */
static void node_color(uint8_t id, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const uint8_t palette[][3] = {
        {0, 64, 0},    /* node 1：绿 */
        {0, 0, 64},    /* node 2：蓝 */
        {64, 48, 0},   /* node 3：黄 */
        {64, 0, 0},    /* node 4：红 */
        {0, 48, 64},   /* node 5：青 */
        {48, 0, 64},   /* node 6：紫 */
        {64, 64, 0},   /* node 7：暖白 */
        {64, 64, 64},  /* node 8：白 */
    };
    const unsigned count = sizeof(palette) / sizeof(palette[0]);
    /* id 从 1 开始；异常值统一按第一种颜色处理，避免负下标 */
    const unsigned idx = (id >= 1) ? (unsigned)(id - 1) : 0u;
    const uint8_t *c = palette[idx % count];

    *r = c[0];
    *g = c[1];
    *b = c[2];
}

/* 运行在 Wi-Fi 任务里：只入队，绝不阻塞 */
static void on_espnow_recv(const espnow_packet_t *packet, const uint8_t src_mac[6])
{
    if (packet->event != ESPNOW_EVENT_PRESS) {
        return;
    }
    /* 编号超出范围的数据帧直接丢弃，避免污染去重表和调色板 */
    if (packet->device_id < 1 || packet->device_id > HUB_MAX_NODES) {
        return;
    }

    hub_event_t event = {
        .device_id = packet->device_id,
        .seq = packet->seq,
    };
    memcpy(event.mac, src_mac, sizeof(event.mac));

    if (xQueueSend(s_event_queue, &event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full, dropping event from node %u",
                 (unsigned)event.device_id);
    }
}

static bool is_duplicate(const hub_event_t *event)
{
    if (event->device_id < 1 || event->device_id > HUB_MAX_NODES) {
        return false;
    }

    const int i = event->device_id - 1;
    const int64_t now = esp_timer_get_time();
    const bool duplicate = (s_last_seq[i] == event->seq) &&
                           (now - s_last_seq_time[i] < DEDUP_WINDOW_US);

    s_last_seq[i] = event->seq;
    s_last_seq_time[i] = now;
    return duplicate;
}

static void report_event(const hub_event_t *event)
{
    uint8_t r, g, b;
    node_color(event->device_id, &r, &g, &b);
    status_led_set_color(r, g, b);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Event from node %u (seq=%u)",
             (unsigned)event->device_id, (unsigned)event->seq);
    ESP_LOGI(TAG, "Source MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             event->mac[0], event->mac[1], event->mac[2],
             event->mac[3], event->mac[4], event->mac[5]);
    ESP_LOGI(TAG, "Onboard LED blinking for 3 s ...");
}

static void hub_task(void *arg)
{
    status_led_set(false);

    int64_t blink_until = 0;
    int64_t next_toggle = 0;
    bool led_on = false;

    for (;;) {
        hub_event_t event;
        /* 100ms 超时同时充当闪烁节拍 */
        BaseType_t got = xQueueReceive(s_event_queue, &event,
                                       pdMS_TO_TICKS(BLINK_INTERVAL_US / 1000));
        int64_t now = esp_timer_get_time();

        if (got == pdTRUE && !is_duplicate(&event)) {
            report_event(&event);
            /* 从最新事件重新计时 */
            blink_until = now + BLINK_DURATION_US;
        }

        now = esp_timer_get_time();
        if (blink_until != 0) {
            if (now < blink_until) {
                if (now >= next_toggle) {
                    led_on = !led_on;
                    status_led_set(led_on);
                    next_toggle = now + BLINK_INTERVAL_US;
                }
            } else {
                status_led_set(false);
                led_on = false;
                blink_until = 0;
                ESP_LOGI(TAG, "Blink finished, listening again ...");
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP-NOW hub starting (ESP32-S3)");

    s_event_queue = xQueueCreate(16, sizeof(hub_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        abort();
    }

    for (int i = 0; i < HUB_MAX_NODES; i++) {
        s_last_seq[i] = 0xFFFF;
    }

    ESP_ERROR_CHECK(espnow_link_init());
    ESP_ERROR_CHECK(status_led_init(CONFIG_HUB_LED_GPIO, HUB_LED_TYPE,
                                    HUB_LED_ACTIVE_LOW, 0, 64, 0));
    ESP_ERROR_CHECK(espnow_link_set_rx_cb(on_espnow_recv));

    BaseType_t ret = xTaskCreate(hub_task, "hub_task", 4096, NULL, 5, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create hub task");
        abort();
    }

    ESP_LOGI(TAG, "Ready. Waiting for node events (node id 1 ~ %d)", HUB_MAX_NODES);
}
