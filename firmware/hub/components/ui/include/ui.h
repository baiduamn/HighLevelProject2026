#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UI 层：只依赖 bsp 和 LVGL，不直接操作面板驱动。
 * 现在只是桩，主链路（ESP-NOW → 事件队列 → hub_task → ui）已经接好。
 */

/** 初始化 UI（LVGL 就绪后在这里创建界面） */
esp_err_t ui_init(void);

/** 某个从机上报了一个事件，交给 UI 更新显示 */
void ui_update_node(uint8_t node_id, uint16_t seq);

#ifdef __cplusplus
}
#endif
