/*
 * ESP-NOW node（ESP32-S3）—— 从机
 *
 * 按下按键后通过 ESP-NOW 广播上报，主机收到后显示并闪灯。
 * 每次按键连发 3 个相同的包（广播无 ACK），主机按 (device_id, seq) 去重。
 *
 * 后续接入传感器时：把 simple_button_poll() 换成传感器采样，
 * 采集到数据后调用 espnow_link_send_*() 上报即可，其余结构不用动。
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espnow_link.h"
#include "simple_button.h"
#include "status_led.h"

static const char *TAG = "node";

#ifdef CONFIG_NODE_LED_TYPE_WS2812
#define NODE_LED_TYPE STATUS_LED_WS2812
#else
#define NODE_LED_TYPE STATUS_LED_GPIO
#endif

#ifdef CONFIG_NODE_LED_ACTIVE_LOW
#define NODE_LED_ACTIVE_LOW true
#else
#define NODE_LED_ACTIVE_LOW false
#endif

#ifdef CONFIG_NODE_BUTTON_ACTIVE_LOW
#define NODE_BUTTON_ACTIVE_LOW true
#else
#define NODE_BUTTON_ACTIVE_LOW false
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 node starting, device id: %d", CONFIG_NODE_DEVICE_ID);

    ESP_ERROR_CHECK(espnow_link_init());
    ESP_ERROR_CHECK(status_led_init(CONFIG_NODE_LED_GPIO, NODE_LED_TYPE,
                                    NODE_LED_ACTIVE_LOW, 0, 0, 32));

    simple_button_t button;
    ESP_ERROR_CHECK(simple_button_init(&button, CONFIG_NODE_BUTTON_GPIO,
                                       NODE_BUTTON_ACTIVE_LOW));

    ESP_LOGI(TAG, "Ready. Waiting for button press (GPIO%d)...", CONFIG_NODE_BUTTON_GPIO);

    uint16_t seq = 0;

    for (;;) {
        if (simple_button_poll(&button)) {
            seq++;
            ESP_LOGI(TAG, "Button pressed -> sending ESP-NOW (seq=%u)", seq);

            /* 发送时点亮 LED 作为本地指示 */
            status_led_set(true);
            esp_err_t err = espnow_link_send_press(CONFIG_NODE_DEVICE_ID, seq, 3, 20);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "ESP-NOW send failed: %s", esp_err_to_name(err));
            }
            vTaskDelay(pdMS_TO_TICKS(60));
            status_led_set(false);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
