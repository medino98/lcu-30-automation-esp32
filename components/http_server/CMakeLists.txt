idf_component_register(SRCS "src/basic_http_server.c" "src/basic_auth.c"
                    PRIV_REQUIRES esp_http_server json vfs esp-tls spiffs
                    INCLUDE_DIRS "include")

spiffs_create_partition_image(storage ../../assets/pages FLASH_IN_PROJECT)