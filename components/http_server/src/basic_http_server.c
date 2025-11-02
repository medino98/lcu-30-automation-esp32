#include "basic_http_server.h"

#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"s
#include "cJSON.h"
#include "basic_auth.h"
#include "esp_spiffs.h"


static const char *REST_TAG = "rest-server";

/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define WEB_MOUNT_POINT "/assets/pages"

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* add persistent server/context handles so stop_rest_server can free context */
static httpd_handle_t s_server_handle = NULL;
static rest_server_context_t *s_rest_context = NULL;

esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(REST_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(REST_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(REST_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(REST_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(REST_TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

/* Helper function to process POST requests into a string*/
static esp_err_t process_post_helper(httpd_req_t *req, char* buf)
{
    int total_len = req->content_len;
    int cur_len = 0;
    buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long!");
        buf = NULL;
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post input!");
            memset(buf,0,SCRATCH_BUFSIZE);
            buf = NULL;
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    return ESP_OK;
}

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    if (basic_auth_handler(req) != ESP_OK) {
        // Authentication failed, response already sent by basic_auth_handler
        return ESP_FAIL;
    }

    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Send HTTP Response with the user whitelist (key-value: device-MAC address) */
// static esp_err_t whitelist_get_handler(httpd_req_t *req) 
// {
//     httpd_resp_set_type(req, "application/json");
//     cJSON *root = cJSON_CreateObject();

//     rest_server_context_t

// }

esp_err_t start_rest_server(void)
{
    const char *base_path = WEB_MOUNT_POINT;
    REST_CHECK(base_path, "wrong base path", err);

    if (s_server_handle) {
        ESP_LOGW(REST_TAG, "HTTP server already running");
        return ESP_OK;
    }

    s_rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(s_rest_context, "No memory for rest context", err);
    strlcpy(s_rest_context->base_path, base_path, sizeof(s_rest_context->base_path));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&s_server_handle, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching device whitelist */
    // static const httpd_uri_t device_whitelist_uri= {
    //     .uri       = "/api/v1/devices/whitelist",
    //     .method    = HTTP_GET,
    //     .handler   = whitelist_get_handler,
    //     .user_ctx  = rest_context
    // };
    // httpd_register_uri_handler(server, &device_whitelist_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = s_rest_context
    };
    httpd_register_uri_handler(s_server_handle, &common_get_uri);

    return ESP_OK;
err_start:
    free(s_rest_context);
err:
    return ESP_FAIL;
}

esp_err_t stop_rest_server(void)
{
    if (!s_server_handle) {
        ESP_LOGW(REST_TAG, "HTTP server not running");
        return ESP_OK;
    }

    ESP_LOGI(REST_TAG, "Stopping HTTP Server");
    httpd_stop(s_server_handle);
    s_server_handle = NULL;

    if (s_rest_context) {
        free(s_rest_context);
        s_rest_context = NULL;
    }

    return ESP_OK;
}

