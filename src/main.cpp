#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <map>
#include <string>
#include <vector>

#include "config.h"
#include "parser.h"

using namespace std;

static WiFiClient s_wifiClient;
static PubSubClient s_mqttClient(s_wifiClient);
static uint32_t s_timeLastConnect;
static bool s_hasTriedToConnect;
static bool s_mqttConnected;
static uint32_t s_lastBlinkAt;
static Parser s_parser;

static vector<RegValue> s_regvalues;
typedef std::map<string, string> table_t;
static table_t s_units;
static table_t s_values;

static bool timeAtOrAfter(uint32_t t, uint32_t now) {
  return (int32_t)(now - t) >= 0;
}

static void setStatusLED(bool state) {
  digitalWrite(CONF_PIN_LED_STATUS, state);
}

static void setupWiFi() {
  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);
  Serial.printf("Connecting to %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Trying to connect...");
    delay(1000);
    setStatusLED(counter % 2 != 0);
    counter++;
  }
  WiFi.setAutoReconnect(true);
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
  }
}

static void store_and_report(table_t &table, const string &reg,
                             const string &data, const string &base_topic) {
  const auto reported = table.find(reg);
  if (reported == table.end() || reported->second != data) {
    const string topic = base_topic + reg;
    Serial.printf("Reporting %s to topic %s\n", data.c_str(), topic.c_str());
    s_mqttClient.publish(topic.c_str(), data.c_str(), true);
    table[reg] = data;
  }
}

static void reg_value_cb(const RegValue &rv) { s_regvalues.push_back(rv); }

static void frame_start_cb(const std::string &header) {
  Serial.printf("Frame start: %s\n", header.c_str());
  s_regvalues.clear();
}

static void frame_end_cb() {
  Serial.printf("Frame end\n");
  Serial.write(s_parser.frame().c_str());
  for (auto &&rv : s_regvalues) {
    Serial.printf("Reg %s value %s unit %s\n", rv.reg.c_str(), rv.value.c_str(),
                  rv.unit.c_str());
    store_and_report(s_units, rv.reg, rv.unit, MQTT_UNITS_TOPIC);
    store_and_report(s_values, rv.reg, rv.value, MQTT_VALUES_TOPIC);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  pinMode(CONF_PIN_LED_STATUS, OUTPUT);
  setStatusLED(true);

  setupWiFi();
  setupOTA();
  setupMQTT();

  setStatusLED(false);

  Serial2.begin(115200, SERIAL_8N1, CONF_PIN_DATA_RX, -1, true);

  s_parser.reg_value_cb_ = reg_value_cb;
  s_parser.frame_start_cb_ = frame_start_cb;
  s_parser.frame_end_cb_ = frame_end_cb;
}

void loop() {
  ArduinoOTA.handle();
  loopMQTT();

  const uint32_t t = millis();
  if (timeAtOrAfter(s_lastBlinkAt + 2200, t)) {
    s_lastBlinkAt = t;
    setStatusLED(false);
  } else if (timeAtOrAfter(s_lastBlinkAt + 2000, t)) {
    setStatusLED(true);
  }
  while (Serial2.available() > 0) {
    s_parser.feed((char)Serial2.read());
  }
}
