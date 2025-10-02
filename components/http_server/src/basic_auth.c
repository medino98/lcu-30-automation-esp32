#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "basic_auth.h"
#include "esp_log.h"

static const char *TAG = "HTTP_AUTH";

#define HTTPD_401      "401 UNAUTHORIZED"           /*!< HTTP Response 401 */

static char *http_auth_basic(const char *username, const char *password)
{
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));
    digest = calloc(1, 6 + n + 1);
    strcpy(digest, "Basic ");
    esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    free(user_info);
    return digest;
}

/* An HTTP GET handler */
esp_err_t basic_auth_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
    basic_auth_info->username = "esp";
    basic_auth_info->password = "12346";

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
        } else {
            ESP_LOGE(TAG, "No auth value received");
        }

        char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (strncmp(auth_credentials, buf, buf_len)) {
            ESP_LOGE(TAG, "Not authenticated");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
            free(auth_credentials);
            free(buf);
            return ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "Authenticated!");
            // char *basic_auth_resp = NULL;
            // httpd_resp_set_status(req, HTTPD_200);
            // httpd_resp_set_type(req, "application/json");
            // httpd_resp_set_hdr(req, "Connection", "keep-alive");
            // asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
            // httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            // free(basic_auth_resp);
            free(auth_credentials);
            free(buf);
            return ESP_OK;
        }
    } else {
        ESP_LOGE(TAG, "No auth header received");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
        return ESP_FAIL; // <-- Return ESP_FAIL if no header
    }
}

// static httpd_uri_t basic_auth = {
//     .uri       = "/basic_auth",
//     .method    = HTTP_GET,
//     .handler   = basc_auth_get_handler,
// };

// static void httpd_register_basic_auth(httpd_handle_t server)
// {
//     basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
//     basic_auth_info->username = "ESP32";
//     basic_auth_info->password = "ESP32Webserver";
    
//     basic_auth.user_ctx = basic_auth_info;
//     httpd_register_uri_handler(server, &basic_auth);
// }