idf_component_register(SRCS "main.c"
    REQUIRES driver esp_http_client esp_wifi esp_http_client nvs_flash esp_netif
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio)