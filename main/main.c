#include "esp_log.h"
// #include "mdns_service.h"

#include "basic_http_server.h"
#include "dc_bot.h"
#include "softap_sta.h"


static const char *TAG = "LCU-30H Gate Automation";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting services...");

    // // Initilaize MDNS
    // initialise_mdns();

    // Initialize and start WiFi-Station+SoftAP
    start_softap_sta();

    // Initialize SPIFFS filesystem
    ESP_ERROR_CHECK(init_fs());

    // Initialize and start the Discord Bot
    dc_bot_start();
}