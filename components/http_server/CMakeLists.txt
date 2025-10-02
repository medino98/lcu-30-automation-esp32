idf_component_register(SRCS "src/basic_http_server.c" "src/basic_auth.c"
                    PRIV_REQUIRES esp_http_server json vfs esp-tls
                    INCLUDE_DIRS "include")
