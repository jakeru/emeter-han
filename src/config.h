// config.h

#pragma once

// Pins
#define CONF_PIN_LED_STATUS 13
#define CONF_PIN_DATA_RX 16

// WiFi settings
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WIFI_HOSTNAME ""

// MQTT server
#define MQTT_SERVER "pi4.home"
#define MQTT_SERVER_PORT 1883

// MQTT client name and account credentials
#define MQTT_CLIENT_ID "emeter"
#define MQTT_USER ""
#define MQTT_PASSWORD ""

// MQTT topics
#define MQTT_BASE_TOPIC "emeter/"
#define MQTT_STATUS_TOPIC MQTT_BASE_TOPIC "status"
#define MQTT_IP_ADDRESS_TOPIC MQTT_BASE_TOPIC "ip_address"
#define MQTT_DATA_TOPIC MQTT_BASE_TOPIC "data"