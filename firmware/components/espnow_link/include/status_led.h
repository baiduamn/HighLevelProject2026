#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_LED_GPIO,   /* 普通 GPIO 驱动的 LED */
    STATUS_LED_WS2812, /* 板载 WS2812/SK6812 可编程 RGB LED */
} status_led_type_t;

/**
 * @brief 初始化状态 LED
 *
 * @param gpio_num    LED 所在 GPIO
 * @param type        LED 类型
 * @param active_low  普通 LED 是否低电平点亮（WS2812 忽略此参数）
 * @param r,g,b       WS2812 点亮时的颜色
 */
esp_err_t status_led_init(int gpio_num, status_led_type_t type, bool active_low,
                          uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置 WS2812 点亮时的颜色（普通 GPIO LED 忽略此设置）
 */
void status_led_set_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 开/关 LED
 */
void status_led_set(bool on);

#ifdef __cplusplus
}
#endif
