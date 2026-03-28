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

//---------------------------------- MACROS -----------------------------------
#define TAG "mqtt_client_bl"

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static esp_mqtt_client_handle_t s_client      = NULL;
static bool                     s_connected   = false;

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

bool mqtt_client_bl_is_connected(void)
{
    return s_connected;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    switch((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            s_connected = true;
            ESP_LOGI(TAG, "connected to broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "disconnected from broker");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "mqtt error");
            break;

        default:
            break;
    }
}
