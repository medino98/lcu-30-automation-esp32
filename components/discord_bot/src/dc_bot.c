#include "esp_log.h"

#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "estr.h"

#include "basic_http_server.h"

static const char *TAG = "discord-bot";

static discord_handle_t bot;

static void bot_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    discord_event_data_t *data = (discord_event_data_t *)event_data;

    switch (event_id) {
        case DISCORD_EVENT_CONNECTED: {
            discord_session_t *session = (discord_session_t *)data->ptr;

            ESP_LOGI(TAG, "Bot %s#%s connected", session->user->username, session->user->discriminator);
        } break;

        case DISCORD_EVENT_MESSAGE_RECEIVED: {
            discord_message_t *msg = (discord_message_t *)data->ptr;

            ESP_LOGI(TAG,
                "New message (dm=%s, autor=%s#%s, bot=%s, channel=%s, guild=%s, content=%s)",
                !msg->guild_id ? "true" : "false",
                msg->author->username,
                msg->author->discriminator,
                msg->author->bot ? "true" : "false",
                msg->channel_id,
                msg->guild_id ? msg->guild_id : "NULL",
                msg->content);

            esp_err_t err = ESP_FAIL;
            discord_message_t *sent = NULL;
            
            if(msg->content && msg->content[0] != '!') {
                ESP_LOGI(TAG, "Not a command, ignoring");
                break;
            }

            if (msg->content && strcmp(msg->content, "!config start") == 0) {
                start_rest_server();
                discord_message_t reply = { .content = "Web server started", .channel_id = msg->channel_id };
                err = discord_message_send(bot, &reply, &sent);
            } else if (msg->content && strcmp(msg->content, "!config stop") == 0) {
                stop_rest_server();
                discord_message_t reply = { .content = "Web server stopped", .channel_id = msg->channel_id };
                err = discord_message_send(bot, &reply, &sent);
            }

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Message successfully sent");

                if (sent) { // null check because message can be sent but not returned
                    ESP_LOGI(TAG, "Message got ID #%s", sent->id);
                    discord_message_free(sent);
                }
            }
            else {
                ESP_LOGE(TAG, "Fail to send echo message");
            }
        } break;

        case DISCORD_EVENT_MESSAGE_UPDATED: {
            discord_message_t *msg = (discord_message_t *)data->ptr;
            ESP_LOGI(TAG,
                "%s has updated his message (#%s). New content: %s",
                msg->author->username,
                msg->id,
                msg->content);
        } break;

        case DISCORD_EVENT_MESSAGE_DELETED: {
            discord_message_t *msg = (discord_message_t *)data->ptr;
            ESP_LOGI(TAG, "Message #%s deleted", msg->id);
        } break;

        case DISCORD_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Bot logged out");
            break;
    }
}

void dc_bot_start(void)
{
    discord_config_t cfg = { .intents = DISCORD_INTENT_GUILD_MESSAGES | DISCORD_INTENT_MESSAGE_CONTENT};

    bot = discord_create(&cfg);
    ESP_ERROR_CHECK(discord_register_events(bot, DISCORD_EVENT_ANY, bot_event_handler, NULL));
    ESP_ERROR_CHECK(discord_login(bot));
}
