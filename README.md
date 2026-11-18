# ESP-NOW 一主多从系统

一块 **S3 主机** + 若干 **S3 从机**，之间没有任何接线，通过 2.4GHz ESP-NOW 广播通信。

- **hub（主机）**：接收从机上报，显示在 LCD/LVGL 界面上。
  当前阶段先用「串口打印 + 板载 LED 闪灯」验证链路，UI 后续接入。
- **node（从机）**：采集传感器数据并上报。
  当前阶段先用板载按键模拟一次上报，后续把按键换成传感器采样即可。

> 硬件平台统一为 **ESP32-S3**，不再维护 ESP32-C3。

---

## 0. 新同学三步走

机器上只需要装两样东西：**Git for Windows** 和 **Python 3.11**
（安装 Python 时务必勾选 `Add python.exe to PATH`）。

```bat
git clone <仓库地址>
cd highLevelProject
tools\setup.bat        :: 首次约 10-20 分钟，只跑一次
```

`setup.bat` 会自动完成：下载 ESP-IDF v5.5.6 → 安装编译工具链 → 建好 Python 虚拟环境。
之后编译、烧录都直接双击对应脚本即可，不用再碰环境配置。

> - 工具链默认装在 `%USERPROFILE%\.espressif`（约 2GB，每台机器装一次）。
>   想放别处就提前设置环境变量 `IDF_TOOLS_PATH`。
> - 换 ESP-IDF 版本时，同时改 `tools/setup.bat` 里的 `IDF_VER` 和
>   `firmware/cmake/idf_version_check.cmake` 里的 `IDF_VERSION_REQUIRED`，两处保持一致。
> - `esp-idf/` 由 `setup.bat` 下载，**不进仓库**；版本不符时 CMake 会在配置阶段直接报错。

---

## 1. 目录结构

```
highLevelProject/
├── firmware/
│   ├── hub/                      # 主机固件（target: esp32s3）
│   ├── node/                     # 从机固件（target: esp32s3）
│   ├── components/
│   │   └── espnow_link/          # 共享组件：ESP-NOW 封装、按键消抖、状态 LED
│   └── cmake/
│       └── idf_version_check.cmake   # 统一校验 ESP-IDF 版本
├── hardware/
│   ├── pcb/                      # 原理图 / PCB 源文件
│   └── enclosure/                # 3D / 结构文件
├── docs/
│   └── meetings/                 # 组会资料、架构文档
├── tools/
│   ├── env.bat                   # 公共环境变量（被其它脚本调用）
│   ├── setup.bat                 # 一键环境准备（只跑一次）
│   ├── build_all.bat             # 一键编译全部工程
│   ├── flash.bat                 # 烧录 + 打开串口监视器
│   └── idf_cmd.bat               # 打开已激活 ESP-IDF 环境的命令行
├── esp-idf/                      # ESP-IDF v5.5.6（setup.bat 下载，不进仓库）
├── .gitignore
└── README.md
```

`hub` 和 `node` 通过 `EXTRA_COMPONENT_DIRS` 共用 `firmware/components/espnow_link`，
协议、按键和 LED 代码只维护一份。

---

## 2. 角色与默认配置

| 工程 | 角色 | 编号 | 按键 | 板载 LED |
|---|---|---|---|---|
| `firmware/hub` | 接收 / 显示 | — | — | GPIO48，WS2812 RGB |
| `firmware/node` | 采集 / 上报 | 1 ~ 8 | GPIO0（BOOT，按下接地） | GPIO48，WS2812 RGB |

从机编号用 `idf.py menuconfig` 修改：`ESP-NOW 从机配置` → 从机编号。
烧到不同的从机上要改成各自的编号，主机才能区分来源。

主机给每个编号分配一种颜色（1 绿、2 蓝、3 黄、4 红……最多 8 个），
控制台也会打印 `Event from node N`。

---

## 3. 编译与烧录

双击 `tools\build_all.bat`，会依次编译 `hub` 和 `node`。

烧录（COM 口按实际情况替换）：

```bat
tools\flash.bat hub  COM5    :: 主机
tools\flash.bat node COM6    :: 从机
```

`flash.bat` 烧录后会直接打开 `idf.py monitor`，退出监视器按 `Ctrl+]`。

也可以只编译某一个工程：

```bat
cd firmware\hub
idf.py build
```

### 不用 ESP-IDF 烧录

编译后每个工程都会生成 `build/merged.bin`（bootloader + 分区表 + app 已合并），
用任何烧录工具在 **0x0** 写入这一个文件即可：

```bat
esptool.py --chip esp32s3 -p COM5 -b 460800 write_flash 0x0 firmware\hub\build\merged.bin
```

图形工具（Flash Download Tool）选对芯片，在 `0x0` 添加对应的 `merged.bin`，
Flash 配置 DIO / 80MHz / 2MB 即可。

---

## 4. 预期现象

主机上电：

```
I (xxx) espnow_link: Local Wi-Fi MAC: xx:xx:xx:xx:xx:xx, ESP-NOW channel: 1
I (xxx) status_led: LED initialized: GPIO48, type=WS2812
I (xxx) hub: Ready. Waiting for node events (node id 1 ~ 8)
```

从机上电：

```
I (xxx) node: ESP32-S3 node starting, device id: 1
I (xxx) node: Ready. Waiting for button press (GPIO0)...
```

按下从机 BOOT 键，从机打印：

```
I (xxx) node: Button pressed -> sending ESP-NOW (seq=1)
```

主机闪灯并打印：

```
I (xxx) hub: ========================================
I (xxx) hub: Event from node 1 (seq=1)
I (xxx) hub: Source MAC: ...
I (xxx) hub: Onboard LED blinking for 3 s ...
I (xxx) hub: Blink finished, listening again ...
```

---

## 5. 实现要点

- **广播通信**：从机发往 `FF:FF:FF:FF:FF:FF`，主机不需要提前知道从机 MAC；
  所有设备固定 2.4GHz 信道 1，不连路由器。
- **协议包**：8 字节 `magic/version/device_id/event/seq/reserved`，
  见 `firmware/components/espnow_link/include/espnow_link.h`。
  主机按 `magic + version` 过滤无关数据。
- **可靠性**：广播没有 ACK，从机每次上报连发 3 包（间隔 20ms）；
  主机按 `(device_id, seq)` 在 1 秒窗口内去重，所以只处理一次。
- **不阻塞接收**：ESP-NOW 回调运行在 Wi-Fi 任务里，只负责入队；
  闪灯、打印和去重都在独立的 `hub_task` 中完成。
- **LED 驱动**：WS2812 用 ESP-IDF 的 RMT 字节编码器实现，不依赖外部组件。
- **按键**：10~20ms 轮询 + 30ms 消抖，每次按下只产生一次事件。

---

## 6. 常见问题

| 现象 | 排查 |
|---|---|
| 主机 LED 不亮/不闪 | 板载灯不是 GPIO48 的 WS2812。用 menuconfig 改成普通 GPIO LED 并填正确引脚。 |
| 按从机没反应 | 确认是在启动完成后按的；看从机串口是否打印「Button pressed」。 |
| 多个从机被认成同一个 | 检查各自的 `CONFIG_NODE_DEVICE_ID` 是否不同，是否烧错了固件。 |
| 距离稍远就丢包 | ESP-NOW 为 2.4GHz，尽量保持 10~20m 内、避开 USB 3.0 干扰；已做 3 次重发。 |
| 串口中文乱码 | 使用 VSCode 内置终端或先 `chcp 65001`；`idf.py monitor` 按 UTF-8 输出。 |
| 提示 ESP-IDF 版本不符 | 按提示确认 `IDF_PATH`，或删除 `.setup_done` 后重跑 `tools\setup.bat`。 |

---

## 7. 后续计划

1. 从机接入真实传感器（I2C / SPI / ADC），扩展 `espnow_link` 的上报包。
2. 主机接入 LCD + LVGL，把控制台输出换成界面显示。
3. 硬件部分放入 `hardware/pcb` 与 `hardware/enclosure`。
