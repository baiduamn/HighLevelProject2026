#include "sensors.h"

#include "esp_log.h"

static const char *TAG = "sensors";

esp_err_t sensors_init(void)
{
    /*
     * TODO: 确定传感器型号后在这里：
     *   1. 初始化 I2C / SPI / ADC
     *   2. 调用对应组件的初始化函数（依赖写在 idf_component.yml）
     */
    ESP_LOGI(TAG, "sensors stub initialized (no sensor configured yet)");
    return ESP_OK;
}

esp_err_t sensors_read(sensor_sample_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* TODO: 读取真实传感器数据 */
    return ESP_ERR_NOT_SUPPORTED;
}
