idf_component_register(
    SRCS "espnow_link.c" "status_led.c" "simple_button.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif nvs_flash esp_event esp_timer driver
)
