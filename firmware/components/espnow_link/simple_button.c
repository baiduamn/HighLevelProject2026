#include "simple_button.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define DEBOUNCE_US (30 * 1000) /* 30ms 消抖 */

static bool read_pressed(const simple_button_t *btn)
{
    int level = gpio_get_level(btn->gpio_num);
    return btn->active_low ? (level == 0) : (level != 0);
}

esp_err_t simple_button_init(simple_button_t *btn, int gpio_num, bool active_low)
{
    if (btn == NULL || gpio_num < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << gpio_num,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_cfg);
    if (err != ESP_OK) {
        return err;
    }

    btn->gpio_num = gpio_num;
    btn->active_low = active_low;
    btn->stable = read_pressed(btn);
    btn->last_raw = btn->stable;
    btn->last_change_us = esp_timer_get_time();

    return ESP_OK;
}

bool simple_button_poll(simple_button_t *btn)
{
    if (btn == NULL) {
        return false;
    }

    bool raw = read_pressed(btn);
    int64_t now = esp_timer_get_time();

    if (raw != btn->last_raw) {
        btn->last_raw = raw;
        btn->last_change_us = now;
        return false;
    }

    /* 电平保持稳定超过消抖时间，才认可状态变化 */
    if (raw != btn->stable && (now - btn->last_change_us) >= DEBOUNCE_US) {
        btn->stable = raw;
        return raw; /* 只有“按下”事件返回 true */
    }

    return false;
}
