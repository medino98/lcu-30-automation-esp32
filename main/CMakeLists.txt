idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES http_server mdns_service discord_bot softap_sta
                    INCLUDE_DIRS ".") 
