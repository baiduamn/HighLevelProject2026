#include "status_led.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

static const char *TAG = "status_led";

#define WS2812_RESOLUTION_HZ  (10 * 1000 * 1000) /* 10 MHz -> 1 tick = 0.1us */

typedef struct {
    bool initialized;
    status_led_type_t type;
    int gpio_num;
    bool active_low;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    rmt_channel_handle_t tx_chan;
    rmt_encoder_handle_t encoder;
} status_led_t;

static status_led_t s_led;

static esp_err_t ws2812_init(int gpio_num)
{
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = gpio_num,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = WS2812_RESOLUTION_HZ,
        .mem_block_symbols = 48,
        .trans_queue_depth = 4,
    };
    esp_err_t err = rmt_new_tx_channel(&tx_cfg, &s_led.tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(err));
        return err;
    }

    err = rmt_enable(s_led.tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(err));
        return err;
    }

    /* WS2812 时序（10MHz 分辨率）：bit0 = 0.3us 高 + 0.9us 低，bit1 = 0.9us 高 + 0.3us 低 */
    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 3,
            .level1 = 0,
            .duration1 = 9,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 9,
            .level1 = 0,
            .duration1 = 3,
        },
        .flags.msb_first = 1,
    };
    err = rmt_new_bytes_encoder(&bytes_cfg, &s_led.encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT encoder: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t status_led_init(int gpio_num, status_led_type_t type, bool active_low,
                          uint8_t r, uint8_t g, uint8_t b)
{
    if (gpio_num < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_led, 0, sizeof(s_led));
    s_led.type = type;
    s_led.gpio_num = gpio_num;
    s_led.active_low = active_low;
    s_led.r = r;
    s_led.g = g;
    s_led.b = b;

    esp_err_t err;
    if (type == STATUS_LED_WS2812) {
        err = ws2812_init(gpio_num);
    } else {
        gpio_config_t io_cfg = {
            .pin_bit_mask = 1ULL << gpio_num,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        err = gpio_config(&io_cfg);
        if (err == ESP_OK) {
            /* 初始熄灭 */
            gpio_set_level(gpio_num, active_low ? 1 : 0);
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LED init failed (GPIO%d): %s", gpio_num, esp_err_to_name(err));
        return err;
    }

    s_led.initialized = true;
    ESP_LOGI(TAG, "LED initialized: GPIO%d, type=%s", gpio_num,
             type == STATUS_LED_WS2812 ? "WS2812" : "GPIO");
    return ESP_OK;
}

void status_led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    s_led.r = r;
    s_led.g = g;
    s_led.b = b;
}

void status_led_set(bool on)
{
    if (!s_led.initialized) {
        return;
    }

    if (s_led.type == STATUS_LED_GPIO) {
        gpio_set_level(s_led.gpio_num, s_led.active_low ? !on : on);
        return;
    }

    /* WS2812 数据顺序为 G-R-B */
    uint8_t grb[3] = {
        on ? s_led.g : 0,
        on ? s_led.r : 0,
        on ? s_led.b : 0,
    };
    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };
    esp_err_t err = rmt_transmit(s_led.tx_chan, s_led.encoder, grb, sizeof(grb), &tx_cfg);
    if (err == ESP_OK) {
        /* 数据只有 24bit，等待几十微秒即可完成 */
        rmt_tx_wait_all_done(s_led.tx_chan, 100);
    }
}
