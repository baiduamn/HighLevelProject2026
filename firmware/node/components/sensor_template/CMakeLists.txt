# ============================================================
#  驱动模板：复制整个目录开始写一个新传感器驱动
#
#    1. 复制本目录，例如改成 sensor_sht3x/
#    2. 把文件名、代码里的 sensor_template 全部改名
#    3. 在 sensors/CMakeLists.txt 的 PRIV_REQUIRES 里加上新组件名
#    4. 在 sensors.c 里调用它的 init / read
#
#  注意：组件名默认等于目录名，所以目录名就是组件名。
# ============================================================
idf_component_register(
    SRCS "sensor_template.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_i2c
)
