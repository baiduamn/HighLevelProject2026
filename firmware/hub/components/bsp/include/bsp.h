#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 板级支持（BSP）：把「本板子的引脚、LCD、触摸、背光」集中在这里。
 * ui 和 main 只能通过本文件的接口访问硬件，换屏 / 改 PCB 时只改 bsp。
 */

/** 初始化本板所有板级外设（当前只做基础初始化） */
esp_err_t bsp_init(void);

/** 背光开关（面板接好后实现） */
esp_err_t bsp_backlight_set(bool on);

#ifdef __cplusplus
}
#endif
