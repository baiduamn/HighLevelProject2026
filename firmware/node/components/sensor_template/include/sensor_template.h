#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 传感器驱动模板。
 *
 * 边界：驱动只负责「和芯片通信、产出物理量」。
 * 不要在这里引用 ESP-NOW、node_id 或上报逻辑——那是 sensors.c 的职责。
 */

typedef struct {
    int     i2c_port;
    uint8_t i2c_addr;
    int     gpio_sda;
    int     gpio_scl;
} sensor_template_config_t;

/** 初始化总线并探测器件 */
esp_err_t sensor_template_init(const sensor_template_config_t *cfg);

/** 读一次数据：温度（摄氏度）和相对湿度（%） */
esp_err_t sensor_template_read(float *temperature_c, float *humidity_rh);

#ifdef __cplusplus
}
#endif
