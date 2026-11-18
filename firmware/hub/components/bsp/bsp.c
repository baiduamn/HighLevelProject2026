#include "bsp.h"

#include "esp_log.h"

static const char *TAG = "bsp";

esp_err_t bsp_init(void)
{
    /*
     * TODO: 面板型号确定后在这里完成：
     *   1. 配置 SPI/I2C 总线
     *   2. 用 esp_lcd_* 初始化面板，拿到 esp_lcd_panel_handle_t
     *   3. 配置背光 GPIO / PWM
     *   4. 提供 bsp_lcd_get_handle() 给 ui 使用
     */
    ESP_LOGI(TAG, "BSP stub initialized (no panel configured yet)");
    return ESP_OK;
}

esp_err_t bsp_backlight_set(bool on)
{
    /* TODO: 面板接好后控制背光 GPIO */
    (void)on;
    return ESP_OK;
}
