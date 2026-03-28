/**
 * @file mqtt_client_bl.h
 *
 * @brief Simple MQTT client wrapper for publishing messages from the GUI.
 *
 * Usage:
 *   1. Call wifi_station_init() first, then mqtt_client_bl_init().
 *   2. From any GUI event handler call mqtt_client_bl_publish().
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef MQTT_CLIENT_BL_H
#define MQTT_CLIENT_BL_H

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"
#include <stdbool.h>

//---------------------------------- MACROS -----------------------------------

/** Broker URI – override in menuconfig or change here. */
#ifndef MQTT_BROKER_URI
#define MQTT_BROKER_URI "mqtt://TvojBroker.com"
#endif

//------------------------- PUBLIC FUNCTION PROTOTYPES ------------------------

/**
 * @brief Start the MQTT client and connect to the broker.
 *        Call this after WiFi has obtained an IP address.
 *
 * @return ESP_OK on success.
 */
esp_err_t mqtt_client_bl_init(void);

/**
 * @brief Publish a message to a topic.
 *
 * @param topic    MQTT topic string, e.g. "/wes/sensor/data".
 * @param payload  Null-terminated string payload.
 * @param qos      QoS level: 0, 1, or 2.
 * @param retain   true to set the retain flag.
 * @return ESP_OK on success, ESP_FAIL if not connected.
 */
esp_err_t mqtt_client_bl_publish(const char *topic, const char *payload,
                                 int qos, bool retain);

/**
 * @brief Returns true if the client is currently connected to the broker.
 */
bool mqtt_client_bl_is_connected(void);

#endif /* MQTT_CLIENT_BL_H */
