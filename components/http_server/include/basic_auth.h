#ifndef BASIC_AUTH
#define BASIC_AUTH

typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;

esp_err_t basic_auth_handler(httpd_req_t *req);

#endif /* BASIC_AUTH */
