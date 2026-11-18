#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 传感器层统一接口。
 * 主任务只调用 sensors_init() / sensors_read()，
 * 具体是哪种传感器由本组件内部实现决定；换传感器不用改 main。
 */

/** 一次采样结果（按需要扩展字段） */
typedef struct {
    float    temperature_c;
    float    humidity_rh;
    uint32_t timestamp_ms;
} sensor_sample_t;

/** 初始化传感器总线（I2C/SPI/ADC）并探测器件 */
esp_err_t sensors_init(void);

/** 读取一次数据 */
esp_err_t sensors_read(sensor_sample_t *out);

#ifdef __cplusplus
}
#endif
