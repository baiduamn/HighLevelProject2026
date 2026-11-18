#include "sensor_template.h"

#include "esp_log.h"

static const char *TAG = "sensor_template";

/*
 * TODO: 需要和芯片通信时，把总线/设备句柄保存在文件内静态变量里，例如：
 *
 *   #include "driver/i2c_master.h"
 *   static i2c_master_bus_handle_t s_bus;
 *   static i2c_master_dev_handle_t s_dev;
 *
 * 对外只暴露 init / read，句柄不要出现在头文件里。
 */

esp_err_t sensor_template_init(const sensor_template_config_t *cfg)
{
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * TODO:
     *   1. 用 i2c_new_master_bus() / i2c_master_bus_add_device()
     *      按 cfg 创建总线并挂上器件
     *   2. 读芯片 ID 寄存器，确认器件在线
     *   3. 写配置寄存器（量程、采样率、工作模式等）
     */
    ESP_LOGI(TAG, "init placeholder: port=%d addr=0x%02X SDA=%d SCL=%d",
             cfg->i2c_port, cfg->i2c_addr, cfg->gpio_sda, cfg->gpio_scl);
    return ESP_OK;
}

esp_err_t sensor_template_read(float *temperature_c, float *humidity_rh)
{
    if (temperature_c == NULL || humidity_rh == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * TODO:
     *   1. 触发一次测量（或读取周期模式下的最新值）
     *   2. 读回原始数据
     *   3. 按数据手册换算成物理量，写入 *temperature_c / *humidity_rh
     *
     * 注意：I2C 传输是阻塞调用，调用方不能在中断或 ESP-NOW 回调里调这里。
     */
    return ESP_ERR_NOT_SUPPORTED;
}
