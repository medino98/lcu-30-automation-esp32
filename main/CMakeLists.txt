idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES esp_http_server http_server nvs_flash mdns_service discord_bot softap_sta
                    INCLUDE_DIRS ".") 
