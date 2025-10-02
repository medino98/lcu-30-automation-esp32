idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES esp_http_server softap http_server nvs_flash spiffs mdns_service
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../assets/pages FLASH_IN_PROJECT)