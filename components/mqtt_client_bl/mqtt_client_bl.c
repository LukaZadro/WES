/**
 * @file mqtt_client_bl.c
 *
 * @brief Simple MQTT client wrapper for publishing messages from the GUI.
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "mqtt_client_bl.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

//---------------------------------- MACROS -----------------------------------
#define TAG "mqtt_client_bl"

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static esp_mqtt_client_handle_t s_client      = NULL;
static bool                     s_connected   = false;
static mqtt_message_cb_t        s_message_cb  = NULL;

/* Topic to auto-subscribe on (re)connect — set by mqtt_client_bl_subscribe(). */
static char s_sub_topic[64] = {0};
static int  s_sub_qos       = 0;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static void _mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data);

//------------------------------ PUBLIC FUNCTIONS -----------------------------

esp_err_t mqtt_client_bl_init(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    s_client = esp_mqtt_client_init(&cfg);
    if(!s_client)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return ESP_FAIL;
    }

    esp_err_t ret = esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                                   _mqtt_event_handler, NULL);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "register_event failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_mqtt_client_start(s_client);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "client_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "connecting to %s", MQTT_BROKER_URI);
    return ESP_OK;
}

esp_err_t mqtt_client_bl_publish(const char *topic, const char *payload,
                                 int qos, bool retain)
{
    if(!s_connected)
    {
        ESP_LOGW(TAG, "not connected, cannot publish");
        return ESP_FAIL;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0,
                                         qos, (int)retain);
    if(msg_id < 0)
    {
        ESP_LOGE(TAG, "publish failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "published to '%s': %s (msg_id=%d)", topic, payload, msg_id);
    return ESP_OK;
}

esp_err_t mqtt_client_bl_subscribe(const char *topic, int qos)
{
    strncpy(s_sub_topic, topic, sizeof(s_sub_topic) - 1);
    s_sub_topic[sizeof(s_sub_topic) - 1] = '\0';
    s_sub_qos = qos;

    if(!s_connected)
    {
        ESP_LOGW(TAG, "not connected — will subscribe to '%s' on reconnect", topic);
        return ESP_OK;
    }

    int msg_id = esp_mqtt_client_subscribe(s_client, topic, qos);
    if(msg_id < 0)
    {
        ESP_LOGE(TAG, "subscribe to '%s' failed", topic);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "subscribed to '%s' qos=%d", topic, qos);
    return ESP_OK;
}

void mqtt_client_bl_set_message_cb(mqtt_message_cb_t cb)
{
    s_message_cb = cb;
}

bool mqtt_client_bl_is_connected(void)
{
    return s_connected;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            s_connected = true;
            ESP_LOGI(TAG, "connected to broker");
            /* Re-subscribe if a topic was registered. */
            if(s_sub_topic[0] != '\0')
            {
                esp_mqtt_client_subscribe(s_client, s_sub_topic, s_sub_qos);
                ESP_LOGI(TAG, "auto-subscribed to '%s'", s_sub_topic);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "disconnected from broker");
            break;

        case MQTT_EVENT_DATA:
            if(s_message_cb && event->data_len > 0)
            {
                /* Null-terminate topic and payload (MQTT data is not null-terminated). */
                char *payload = malloc(event->data_len + 1);
                if(payload)
                {
                    memcpy(payload, event->data, event->data_len);
                    payload[event->data_len] = '\0';

                    char *topic = malloc(event->topic_len + 1);
                    if(topic)
                    {
                        memcpy(topic, event->topic, event->topic_len);
                        topic[event->topic_len] = '\0';
                        s_message_cb(topic, payload);
                        free(topic);
                    }
                    free(payload);
                }
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "mqtt error");
            break;

        default:
            break;
    }
}
