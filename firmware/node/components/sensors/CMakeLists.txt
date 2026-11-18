idf_component_register(
    SRCS "sensors.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_i2c esp_driver_spi esp_adc
)
