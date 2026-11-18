# ESP-NOW 三机按键联动系统架构

> 三块开发板：**S3 主机**、**S3 发送端（id=1）**、**C3 mini 发送端（id=2）**。
> 三块板之间无任何接线，通过 2.4GHz ESP-NOW 广播通信；各自通过 USB 供电和查看串口。
> 运行环境：ESP-IDF v5.5.6 + FreeRTOS。

---

## 1. 系统总体框图

```mermaid
graph TB
    subgraph HUB["S3 主机（接收 + 指示）"]
        H_WIFI["Wi-Fi task<br/>优先级 23<br/>ESP-NOW 回调"]
        H_QUEUE["事件队列<br/>16 × hub_event_t"]
        H_TASK["hub_task<br/>优先级 5<br/>业务逻辑"]
        H_LED["WS2812<br/>GPIO48 / RMT"]
        H_LOG["USB 串口<br/>英文日志"]
        H_WIFI -->|"xQueueSend（超时 0）"| H_QUEUE
        H_QUEUE -->|"xQueueReceive（100ms）"| H_TASK
        H_TASK -->|"绿 / 蓝 闪烁 3s"| H_LED
        H_TASK -->|"ESP_LOGI"| H_LOG
    end

    subgraph S3TX["S3 发送端（id=1）"]
        S3_BTN["BOOT 按键<br/>GPIO0"]
        S3_MAIN["main task<br/>优先级 1"]
        S3_LED["WS2812<br/>GPIO48"]
        S3_BTN -->|"10ms 轮询 + 30ms 消抖"| S3_MAIN
        S3_MAIN -->|"发送时点亮"| S3_LED
    end

    subgraph C3TX["C3 mini 发送端（id=2）"]
        C3_BTN["BOOT 按键<br/>GPIO9"]
        C3_MAIN["main task<br/>优先级 1"]
        C3_LED["LED<br/>GPIO8（低电平亮）"]
        C3_BTN -->|"10ms 轮询 + 30ms 消抖"| C3_MAIN
        C3_MAIN -->|"发送时点亮"| C3_LED
    end

    S3_MAIN -->|"ESP-NOW 广播 ×3<br/>FF:FF:FF:FF:FF:FF"| RF(("2.4GHz<br/>信道 1"))
    C3_MAIN -->|"ESP-NOW 广播 ×3"| RF
    RF -->|"8 字节数据帧"| H_WIFI
```

---

## 2. FreeRTOS 任务模型

> ESP-IDF 中 `xTaskCreate` 的栈大小单位是**字节**；优先级数字**越大越高**（本工程 configMAX_PRIORITIES = 25）。
> S3 是双核（FreeRTOS SMP），C3 是单核（`CONFIG_FREERTOS_UNICORE=y`）。



### 2.1 S3 / C3 发送端（espnow_s3_btn、espnow_c3_btn）

| 任务     | 创建者       |   优先级 | 职责                                    |
| ------ | --------- | ----: | ------------------------------------- |
| `main` | IDF 启动代码  | **1** | `app_main` 里的业务循环：10ms 轮询按键、发广播、点 LED |
| `wifi` | Wi-Fi 协议栈 |    23 | 发送 ESP-NOW 帧、执行发送完成回调                 |


**关键点**

- Wi-Fi 任务（23）优先级高于应用任务（5 或 1），射频事件会抢占业务代码。
- ESP-NOW 接收回调运行在 Wi-Fi 任务上下文，**不能阻塞**，所以回调只做过滤 + `xQueueSend(..., 0)`；真正的打印和闪放在 `hub_task`。

---

## 3. ESP-NOW 收发

### 3.1 数据包（8 字节，广播）//TO DO 需要更新

| 偏移 | 字段          | 长度 | 值 / 含义               |
| ---: | ------------- | ---: | ----------------------- |
|    0 | `magic`       |    1 | `0xE5`，过滤无关帧      |
|    1 | `device_type` |    1 | 传感器设备              |
|    2 | `device_id`   |    1 | 同一种传感器的不同设备  |
|    3 | `event`       |    1 | 数据上报 或 ACK         |
|    4 | `seq`         |    2 | 每次按键 +1，主机去重用 |
|    6 | 数据          |   10 | 数据内容，小端          |

- 目的地址：`FF:FF:FF:FF:FF:FF`（广播），主机无需预知发送端 MAC。
- 所有设备固定 **2.4GHz 信道 1**，不连接路由器。
- 广播没有 ACK/重传，因此每次按键连发 **3 包**（间隔 20ms）。

### 3.2 发送端数据流

```mermaid
flowchart TD
    A["main task：每 10ms 轮询按键"] --> B{"电平稳定按下 ≥ 30ms?"}
    B -->|"否"| A
    B -->|"是"| C["seq++<br/>组装 8 字节包"]
    C --> D["LED 点亮（本地提示）"]
    D --> E["esp_now_send 广播<br/>重复 3 次，间隔 20ms"]
    E --> F["vTaskDelay(60ms)"]
    F --> G["LED 熄灭"]
    G --> A
```

### 3.3 主机接收数据流

```mermaid
flowchart TD
    A["Wi-Fi task（23）收到 ESP-NOW 帧"] --> B{"长度 ≥ 8 且<br/>magic/version 正确?"}
    B -->|"否"| X["丢弃"]
    B -->|"是"| C["xQueueSend 事件队列<br/>超时 0，不阻塞"]
    C --> D["hub_task（5）<br/>xQueueReceive 100ms"]
    D --> E{"同一 device_id + seq<br/>1 秒内重复?"}
    E -->|"是"| F["忽略（3 连包只处理 1 次）"]
    E -->|"否"| G["选择颜色：S3 绿 / C3 蓝"]
    G --> H["串口打印来源、id、seq、MAC"]
    H --> I["blink_until = now + 3s"]
    I --> J["每轮循环：若在闪灯窗口内<br/>则每 100ms 翻转 LED"]
    J --> K{"now ≥ blink_until?"}
    K -->|"否"| D
    K -->|"是"| L["LED 熄灭<br/>打印 Blink finished"]
    F --> D
    L --> D
```



