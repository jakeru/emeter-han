#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"
#include "message.h"

static void callbackForMQTT(char *topic, byte *bytes, unsigned int length);

static WiFiClient s_wifiClient;
static PubSubClient s_mqttClient(s_wifiClient);
static uint32_t s_timeLastConnect;
static bool s_hasTriedToConnect;
static bool s_mqttConnected;
static uint32_t s_lastDataAt;
static MessageBuffer s_msgbuf;
static uint32_t s_lastBlinkAt;

static bool timeAtOrAfter(uint32_t t, uint32_t now) {
  return (int32_t)(now - t) >= 0;
}

static void setupWiFi() {
  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);
  Serial.printf("Connecting to %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

static void setupOTA() {
  ArduinoOTA.onStart([]() { Serial.println("OTA started"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA completed"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\nOTA Error: #%u: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
    else
      Serial.println("Unknown error");
  });
  ArduinoOTA.begin();
}

static void publishStatus() {
  s_mqttClient.publish(MQTT_STATUS_TOPIC, "online", true);
}

static void setupMQTT() {
  s_mqttClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
  s_mqttClient.setCallback(callbackForMQTT);
}

static bool connectMQTT() {
  Serial.printf("Connecting to mqtt %s:%d...\n", MQTT_SERVER, MQTT_SERVER_PORT);
  if (!s_mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                            MQTT_STATUS_TOPIC, 0, 1, "offline")) {
    Serial.print("Failed to connect to MQTT server: ");
    Serial.println(s_mqttClient.state());
    return false;
  }
  s_mqttConnected = true;
  Serial.println("Connected to MQTT server");
  publishStatus();
  s_mqttClient.publish(MQTT_IP_ADDRESS_TOPIC, WiFi.localIP().toString().c_str(),
                       true);

  return true;
}

static void loopMQTT() {
  uint32_t now = millis();
  s_mqttClient.loop();
  if (!s_mqttClient.connected()) {
    if (s_mqttConnected) {
      Serial.println("Lost connection with MQTT server");
      s_mqttConnected = false;
    }
    if (!s_hasTriedToConnect || timeAtOrAfter(s_timeLastConnect + 60000, now)) {
      connectMQTT();
      s_hasTriedToConnect = true;
      s_timeLastConnect = now;
    }
    return;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  pinMode(CONF_PIN_LED_STATUS, OUTPUT);
  digitalWrite(CONF_PIN_LED_STATUS, HIGH);

  setupWiFi();
  setupOTA();
  setupMQTT();

  Serial1.begin(115200, SERIAL_8N1, CONF_PIN_DATA_RX);
}

static void callbackForMQTT(char *topic, byte *bytes, unsigned int length) {}

void loop() {
  ArduinoOTA.handle();
  loopMQTT();

  uint32_t t = millis();
  if (timeAtOrAfter(s_lastBlinkAt + 2000, t)) {
    digitalWrite(CONF_PIN_LED_STATUS, HIGH);
  }
  if (timeAtOrAfter(s_lastBlinkAt + 2200, t)) {
    s_lastBlinkAt = t;
    digitalWrite(CONF_PIN_LED_STATUS, LOW);
  }
  while (Serial1.available() > 0) {
    s_msgbuf.append((char)Serial1.read());
    s_lastDataAt = millis();
  }

  if (!s_msgbuf.str().empty() && timeAtOrAfter(s_lastDataAt + 1000, millis())) {
    Serial.printf("Publishing %zu B:", s_msgbuf.str().size());
    Serial.println(s_msgbuf.str().c_str());
    s_mqttClient.publish(MQTT_DATA_TOPIC, s_msgbuf.str().c_str(), true);
    s_msgbuf.clear();
  }
}
