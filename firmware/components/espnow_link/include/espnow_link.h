#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 所有设备使用的固定 Wi-Fi 信道，必须一致 */
#define ESPNOW_LINK_CHANNEL 1

#define ESPNOW_LINK_MAGIC   0xE5
#define ESPNOW_LINK_VERSION 1

/** 应用层事件类型 */
typedef enum {
    ESPNOW_EVENT_PRESS = 1, /* 按键按下 */
} espnow_link_event_t;

/**
 * 线上数据包（8 字节）。
 * 从机发往广播地址，因此主机不需要预先知道从机的 MAC。
 */
typedef struct __attribute__((packed)) {
    uint8_t  magic;      /* 固定 ESPNOW_LINK_MAGIC，用于过滤无关数据 */
    uint8_t  version;    /* 协议版本 */
    uint8_t  device_id;  /* 从机编号（1 ~ 8），主机据此区分来源 */
    uint8_t  event;      /* espnow_link_event_t */
    uint16_t seq;        /* 消息序号，用于主机去重（从机每上报一次 +1） */
    uint16_t reserved;
} espnow_packet_t;

/** 接收回调在 Wi-Fi 任务上下文执行，禁止阻塞和耗时操作 */
typedef void (*espnow_link_rx_cb_t)(const espnow_packet_t *packet, const uint8_t src_mac[6]);

/**
 * @brief 初始化 NVS / Wi-Fi(STA) / ESP-NOW，并注册广播对端
 */
esp_err_t espnow_link_init(void);

/**
 * @brief 注册接收回调（回调中请只做入队等轻量操作）
 */
esp_err_t espnow_link_set_rx_cb(espnow_link_rx_cb_t cb);

/**
 * @brief 广播一条“按键按下”消息
 *
 * @param device_id 从机编号
 * @param seq       按键序号
 * @param repeats   重复发送次数（广播无 ACK，重复发送提高到达率）
 * @param gap_ms    每次重复之间的间隔
 */
esp_err_t espnow_link_send_press(uint8_t device_id, uint16_t seq, int repeats, uint32_t gap_ms);

#ifdef __cplusplus
}
#endif
