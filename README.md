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
│   │   ├── components/
│   │   │   ├── bsp/              # 板级支持：引脚、LCD、触摸、背光
│   │   │   └── ui/               # LVGL 界面层（依赖自动下载）
│   │   ├── main/
│   │   ├── partitions.csv        # 自定义分区表（app 3MB）
│   │   └── sdkconfig.defaults
│   ├── node/                     # 从机固件（target: esp32s3）
│   │   ├── components/
│   │   │   ├── sensors/          # 传感器统一接口（驱动按型号加）
│   │   │   └── sensor_template/  # 自己写驱动的模板，复制后改名
│   │   ├── main/
│   │   └── sdkconfig.defaults
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

组件按作用域分层，ESP-IDF 会自动扫描 `<工程>/main`、`<工程>/components`
和 `EXTRA_COMPONENT_DIRS`，**不需要改任何 CMakeLists**：

| 目录 | 范围 | 放什么 |
|---|---|---|
| `firmware/components/` | 主从共享 | 协议、ESP-NOW、通用工具 |
| `firmware/hub/components/` | 仅主机 | `bsp`（引脚/屏幕）、`ui`（LVGL） |
| `firmware/node/components/` | 仅从机 | 传感器驱动 |

---

## 1.1 驱动与第三方库（自动下载）

第三方组件走 ESP-IDF 组件管理器，**写一行依赖，构建时自动下载**，
不要把源码抄进仓库：

```yaml
# firmware/hub/components/ui/idf_component.yml
dependencies:
  idf: ">=5.4"
  lvgl/lvgl: "^9"
  espressif/esp_lvgl_port: "^2"
```

```yaml
# firmware/node/components/sensors/idf_component.yml
dependencies:
  idf: ">=5.4"
  # 选好型号后加一行，例如：
  # espressif/aht20: "^1"
```

- 组件仓库：<https://components.espressif.com>
- `managed_components/` 由构建生成，**不提交**
- `dependencies.lock` **要提交**，用来锁死版本
- 需要改源码的驱动，fork 后放到对应工程的 `components/` 下，并改个不重名的名字

---

## 1.2 自己写传感器驱动

模板在 [`firmware/node/components/sensor_template/`](firmware/node/components/sensor_template/)，
按它的 `README.md` 五步操作即可。

核心结构：

```
firmware/node/components/
├── sensors/               # 统一接口：sensors_init() / sensors_read()
├── sensor_sht3x/          # 一颗传感器一个组件（直接子目录！）
│   ├── CMakeLists.txt
│   ├── Kconfig
│   ├── include/sensor_sht3x.h
│   └── sensor_sht3x.c
└── sensor_bmp280/
```

几条硬规则：

- **ESP-IDF 只扫描 `components/` 的下一层**。`components/sensors/sht3x/` 这种嵌套
  不会被识别，驱动必须是 `components/` 的直接子目录。
- **组件名 = 目录名**，起名别和组件仓库里的同名（如 `lvgl`、`esp_lcd_*`）。
- 驱动只负责和芯片通信，**不引用 ESP-NOW / `node_id` / 上报逻辑**。
- 只在 `.c` 里用到的依赖写 `PRIV_REQUIRES`，避免上层重新编译。
- I2C/SPI 传输是阻塞的，**不要放在中断或 ESP-NOW 回调里**。
- 厂商给的源码也可以套进这个结构，保留原始 LICENSE。

对接统一接口（换传感器时 `main` 不用动）：

```cmake
# sensors/CMakeLists.txt
idf_component_register(
    SRCS "sensors.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES sensor_sht3x
)
```

```c
// sensors.c
#include "sensor_sht3x.h"

esp_err_t sensors_read(sensor_sample_t *out)
{
    ESP_RETURN_ON_ERROR(sht3x_read(&out->temperature_c, &out->humidity_rh),
                        TAG, "read failed");
    return ESP_OK;
}
```

> 自己写的驱动是**仓库源码**，正常提交；从组件管理器下载的放
> `managed_components/`，不提交。两者的区别就这一条。

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

> 改了 `sdkconfig.defaults` 或 `partitions.csv` 后，必须删除该工程的 `sdkconfig`
> （或执行 `idf.py fullclean`），否则旧配置不会更新。

### 不用 ESP-IDF 烧录

`build_all.bat` 会在编译后自动执行 `idf.py merge-bin`，每个工程生成
`build/merged.bin`（bootloader + 分区表 + app 已合并），
用任何烧录工具在 **0x0** 写入这一个文件即可：

> 单独执行 `idf.py build` 时**不会**自动合并，需要再跑一次
> `idf.py merge-bin -o merged.bin`。

```bat
esptool.py --chip esp32s3 -p COM5 -b 460800 write_flash 0x0 firmware\hub\build\merged.bin
```

图形工具（Flash Download Tool）选对芯片，在 `0x0` 添加对应的 `merged.bin`，
Flash 配置 DIO / 80MHz / **4MB** 即可。

### 分区表

主机使用自定义分区表 [`firmware/hub/partitions.csv`](firmware/hub/partitions.csv)：

| 分区 | 偏移 | 大小 | 用途 |
|---|---|---|---|
| nvs | 0x9000 | 24 KB | 参数存储 |
| phy_init | 0xF000 | 4 KB | 射频校准 |
| factory | 0x10000 | **3 MB** | 应用（原来只有 1 MB，LVGL 放不下） |

Flash 按 **4MB** 配置（S3 模组最小 4MB，8/16MB 板子也能用）。
如果以后要 OTA，可以改成双 app 分区；要存图片/配置，可以在 0x310000 之后加 `storage` 分区。

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
