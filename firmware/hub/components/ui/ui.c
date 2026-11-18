#include "ui.h"

#include "esp_log.h"

static const char *TAG = "ui";

esp_err_t ui_init(void)
{
    /*
     * TODO: 面板接好后按顺序实现：
     *   1. bsp_init() 拿到面板句柄
     *   2. lvgl_port_init(&cfg)
     *   3. lvgl_port_add_disp(&disp_cfg)
     *   4. 创建界面控件
     *
     * LVGL 不是线程安全的：主任务更新界面时要先 lvgl_port_lock(0)，
     * 更新完再 lvgl_port_unlock()。
     */
    ESP_LOGI(TAG, "UI stub initialized (LVGL screen not started yet)");
    return ESP_OK;
}

void ui_update_node(uint8_t node_id, uint16_t seq)
{
    /* TODO: 加锁后把 node_id / seq 更新到 LVGL 控件上 */
    ESP_LOGI(TAG, "node %u reported seq %u",
             (unsigned)node_id, (unsigned)seq);
}
