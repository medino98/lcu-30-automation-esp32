#ifndef BASIC_HTTP_SERVER
#define BASIC_HTTP_SERVER

#include "esp_err.h"

esp_err_t start_rest_server(void);
esp_err_t stop_rest_server(void);
esp_err_t init_fs(void);


#endif /* BASIC_HTTP_SERVER */
