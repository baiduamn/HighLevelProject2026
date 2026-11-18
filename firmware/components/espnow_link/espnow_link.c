#include "espnow_link.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "espnow_link";

static const uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static espnow_link_rx_cb_t s_rx_cb = NULL;

static esp_err_t nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int data_len)
{
    if (s_rx_cb == NULL || info == NULL || data == NULL) {
        return;
    }
    if (data_len < (int)sizeof(espnow_packet_t)) {
        return;
    }

    const espnow_packet_t *packet = (const espnow_packet_t *)data;
    if (packet->magic != ESPNOW_LINK_MAGIC || packet->version != ESPNOW_LINK_VERSION) {
        return;
    }

    /* 回调运行在 Wi-Fi 任务中：这里只做过滤并转交给上层 */
    s_rx_cb(packet, info->src_addr);
}

static void espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    if (tx_info == NULL) {
        return;
    }
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGD(TAG, "Send failed: " MACSTR, MAC2STR(tx_info->des_addr));
    }
}

esp_err_t espnow_link_init(void)
{
    esp_err_t err = nvs_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* 不连接路由器，固定信道保证所有设备能在同一信道收发 */
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_LINK_CHANNEL, WIFI_SECOND_CHAN_NONE));
    /* 关闭省电，降低接收延迟（设备由 USB 供电，无需省电） */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    if (!esp_now_is_peer_exist(s_broadcast_mac)) {
        esp_now_peer_info_t peer = {
            .channel = ESPNOW_LINK_CHANNEL,
            .ifidx = WIFI_IF_STA,
            .encrypt = false,
        };
        memcpy(peer.peer_addr, s_broadcast_mac, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    }

    uint8_t mac[6];
    err = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Local Wi-Fi MAC: " MACSTR ", ESP-NOW channel: %d",
                 MAC2STR(mac), ESPNOW_LINK_CHANNEL);
    }

    return ESP_OK;
}

esp_err_t espnow_link_set_rx_cb(espnow_link_rx_cb_t cb)
{
    s_rx_cb = cb;
    return ESP_OK;
}

esp_err_t espnow_link_send_press(uint8_t device_id, uint16_t seq, int repeats, uint32_t gap_ms)
{
    if (repeats < 1) {
        repeats = 1;
    }

    espnow_packet_t packet = {
        .magic = ESPNOW_LINK_MAGIC,
        .version = ESPNOW_LINK_VERSION,
        .device_id = device_id,
        .event = ESPNOW_EVENT_PRESS,
        .seq = seq,
        .reserved = 0,
    };

    for (int i = 0; i < repeats; i++) {
        esp_err_t err = esp_now_send(s_broadcast_mac, (const uint8_t *)&packet, sizeof(packet));
        if (err != ESP_OK) {
            return err;
        }
        if (i + 1 < repeats) {
            vTaskDelay(pdMS_TO_TICKS(gap_ms));
        }
    }

    return ESP_OK;
}
