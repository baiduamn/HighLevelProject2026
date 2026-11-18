#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int gpio_num;
    bool active_low;
    bool stable;          /* 消抖后的稳定状态：true = 按下 */
    bool last_raw;        /* 上一次采样值 */
    int64_t last_change_us;
} simple_button_t;

/**
 * @brief 初始化按键 GPIO（内部上拉，低电平有效时）
 *
 * @param active_low true = 按下接地
 */
esp_err_t simple_button_init(simple_button_t *btn, int gpio_num, bool active_low);

/**
 * @brief 轮询按键，建议每 10~20ms 调用一次
 *
 * @return true 表示检测到一次新的“按下”事件（每次按下只返回一次）
 */
bool simple_button_poll(simple_button_t *btn);

#ifdef __cplusplus
}
#endif
