# sensor_template —— 驱动模板

复制这个目录开始写一个新的传感器驱动。

## 五步

1. 复制目录：

   ```
   firmware/node/components/sensor_template
       → firmware/node/components/sensor_sht3x
   ```

2. 把文件名和代码里的 `sensor_template` 全部改成 `sensor_sht3x`
   （组件名默认等于目录名）。

3. 在 `firmware/node/components/sensors/CMakeLists.txt` 的
   `PRIV_REQUIRES` 里加上新组件名：

   ```cmake
   PRIV_REQUIRES sensor_sht3x
   ```

4. 在 `firmware/node/components/sensors/sensors.c` 里调用它的 init / read，
   把数据填进 `sensor_sample_t`。

5. 删掉本模板目录，或留着当参考。

## 规则

- ESP-IDF 只扫描 `components/` 的**下一层**，驱动必须是 `components/` 的
  直接子目录，不能嵌在 `sensors/` 里面。
- 驱动不引用 ESP-NOW、`node_id`、上报逻辑。
- 头文件只暴露 `init` / `read` 这类接口，I2C/SPI 句柄留在 `.c` 里。
- `PRIV_REQUIRES` 用于只在 `.c` 里用到的依赖，避免上层重新编译。
- 厂商给的源码也可以套进这个结构，保留原始 LICENSE。
