// config.h

#pragma once

// Pins
#define CONF_PIN_STATUS_LED 13
#define CONF_PIN_METER_LED 14

#define CONF_PIN_TX 1

#ifndef ESP8266
#define CONF_PIN_RX 3
#endif

// WiFi settings
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WIFI_HOSTNAME "hanif"

// MQTT server
#define MQTT_SERVER "pi4.home"
#define MQTT_SERVER_PORT 1883

// MQTT client name and account credentials
#define MQTT_CLIENT_ID "hanif"
#define MQTT_USER ""
#define MQTT_PASSWORD ""

// MQTT topics
#define MQTT_BASE_TOPIC "hanif/"
#define MQTT_STATUS_TOPIC MQTT_BASE_TOPIC "status"
#define MQTT_IP_ADDRESS_TOPIC MQTT_BASE_TOPIC "ip"
#define MQTT_VALUES_TOPIC MQTT_BASE_TOPIC "values/"
#define MQTT_UNITS_TOPIC MQTT_BASE_TOPIC "units/"
