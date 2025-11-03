#include "esp_log.h"

#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "estr.h"

#include "basic_http_server.h"

static const char *TAG = "discord-bot";

/* forward declarations for handlers (matching typedef) */
static esp_err_t cmd_config(discord_message_t *msg, char *args, discord_message_t **out_result);
static esp_err_t cmd_gate_action(discord_message_t *msg, char *args, discord_message_t **out_result);

/* handler typedef now returns an optional out-result pointer */
typedef esp_err_t (*cmd_handler_t)(discord_message_t *msg, char *args, discord_message_t **out_result);

/* Bot handle*/
static discord_handle_t bot;

/* dispatch table */
static const struct {
    const char *name;
    cmd_handler_t handler;
} commands[] = {
    { "config", cmd_config },
    { "gate", cmd_gate_action },
    { NULL, NULL }
};

static esp_err_t cmd_config(discord_message_t *cmd, char *args, discord_message_t **out_result)
{
    if (!args) return ESP_FAIL;

    if (strcmp(args, "start") == 0) {
        start_rest_server();
        discord_message_t reply = { .content = "Web server started", .channel_id = cmd->channel_id };
        return discord_message_send(bot, &reply, out_result);
    } else if (strcmp(args, "stop") == 0) {
        stop_rest_server();
        discord_message_t reply = { .content = "Web server stopped", .channel_id = cmd->channel_id };
        return discord_message_send(bot, &reply, out_result);
    }

    discord_message_t reply = { .content = "Unknown config subcommand", .channel_id = cmd->channel_id };
    return discord_message_send(bot, &reply, out_result);
}

static esp_err_t cmd_gate_action(discord_message_t *cmd, char *args, discord_message_t **out_result)
{
    if (!args) return ESP_FAIL;

    if (strcmp(args, "open") == 0) {
        // open_gate();
        discord_message_t reply = { .content = "Gate opens fully for car passage!", .channel_id = cmd->channel_id };
        return discord_message_send(bot, &reply, out_result);
    } else if (strcmp(args, "open-half") == 0) {
        // open_gate_halfway();
        discord_message_t reply = { .content = "Gate opens partially for pedestrian passage!", .channel_id = cmd->channel_id };
        return discord_message_send(bot, &reply, out_result);
    } else if (strcmp(args, "close") == 0) {
        // close_gate();
        discord_message_t reply = { .content = "Gate closes!", .channel_id = cmd->channel_id };
        return discord_message_send(bot, &reply, out_result);
    }

    discord_message_t reply = { .content = "Unknown config subcommand", .channel_id = cmd->channel_id };
    return discord_message_send(bot, &reply, out_result);
}

static esp_err_t dc_bot_parse_command(discord_message_t *msg)
{
    /* copy because strtok_r modifies the buffer */
    char buf[128];
    strlcpy(buf, msg->content + 1, sizeof(buf)); /* skip leading '!' */

    char *saveptr = NULL;
    char *cmd = strtok_r(buf, " \t", &saveptr);
    char *args = strtok_r(NULL, "", &saveptr); /* rest of line as args (may be NULL) */

    if (!cmd) return ESP_FAIL;

    discord_message_t *sent = NULL;
    bool matched = false;
    esp_err_t err = ESP_FAIL;

    for (int i = 0; commands[i].name != NULL; ++i) {
        if (strcmp(cmd, commands[i].name) == 0) {
            matched = true;
            err = commands[i].handler(msg, args, &sent);
            break;
        }
    }

    if (!matched) {
        discord_message_t reply = { .content = "Unknown command", .channel_id = msg->channel_id };
        err = discord_message_send(bot, &reply, &sent);
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Message successfully sent");

        if (sent) {
            ESP_LOGI(TAG, "Message got ID #%s", sent->id);
            discord_message_free(sent);
        }
    } else {
        ESP_LOGE(TAG, "Fail to send message");
    }

    return err;
}

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
            
            if(msg->content && msg->content[0] == '!') {
                ESP_LOGI(TAG, "Processing command");
                dc_bot_parse_command(msg);
            }
            else {
                ESP_LOGI(TAG, "Not a command, ignoring");
                break;
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


