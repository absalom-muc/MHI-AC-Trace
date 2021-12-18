#include "support.h"
#include <Arduino.h>

WiFiClient espClient;
PubSubClient MQTTclient(espClient);
int WIFI_lost = 0;
int MQTT_lost = 0;

void initWiFi(){
  WiFi.persistent(false);
  WiFi.disconnect(true);    // Delete SDK wifi config
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME); 
#if UseStrongestAP==true
  WiFi.setAutoReconnect(false);
#else
  WiFi.setAutoReconnect(true);
#endif
}

int networksFound;
void setupWiFi() {
#if UseStrongestAP==true
  int max_rssi = -999;
  int strongest_AP = -1;
  WiFi.scanDelete();
  networksFound = WiFi.scanNetworks();
  for (int i = 0; i < networksFound; i++)
  {
    //Serial.printf("%d: %s, Ch:%d (%ddBm) BSSID:%s %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str(), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
    if((strcmp(WiFi.SSID(i).c_str(), WIFI_SSID) == 0) && (WiFi.RSSI(i)>max_rssi)){
        max_rssi = WiFi.RSSI(i);
        strongest_AP = i;
    }
  }
  Serial.printf_P("current BSSID: %s, strongest BSSID: %s\n", WiFi.BSSIDstr().c_str(), WiFi.BSSIDstr(strongest_AP).c_str());
  if((WiFi.status() != WL_CONNECTED) || ((max_rssi > WiFi.RSSI() + 6) && (strcmp(WiFi.BSSIDstr().c_str(), WiFi.BSSIDstr(strongest_AP).c_str()) != 0))) {
    Serial.printf("Connecting from bssid:%s to bssid:%s, ", WiFi.BSSIDstr().c_str(), WiFi.BSSIDstr(strongest_AP).c_str());
    Serial.printf("channel:%i\n", WiFi.channel(strongest_AP));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WiFi.channel(strongest_AP), WiFi.BSSID(strongest_AP), true);
  }
#else
  Serial.print(F("Attempting WiFi connection ..."));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  unsigned long previousMillis = millis();
  while ((WiFi.status() != WL_CONNECTED) && (millis() - previousMillis < 10*1000)) {
    delay(500);
    Serial.print(F("."));
  }
  if(WiFi.status() == WL_CONNECTED)
    Serial.printf_P(PSTR(" connected to %s, IP address: %s, BSSID: %s\n"), WIFI_SSID, WiFi.localIP().toString().c_str(), WiFi.BSSIDstr().c_str());
  else
    Serial.printf_P(PSTR(" not connected, timeout!\n"));
}

void MQTTreconnect() {
  unsigned long runtimeMillisMQTT;
  char strtmp[50];
  while (!MQTTclient.connected()) { // Loop until we're reconnected
    Serial.print(F("Attempting MQTT connection..."));
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi not connected in MQTTreconnect function"));
        #if UseStrongestAP==true
          setupWiFi();
        #endif
      }
    if (MQTTclient.connect(HOSTNAME, MQTT_USER, MQTT_PASSWORD, MQTT_PREFIX TOPIC_CONNECTED, 0, true, PAYLOAD_CONNECTED_FALSE)) {
      Serial.println(F(" connected"));
      MQTTclient.publish_P(MQTT_PREFIX TOPIC_CONNECTED, PSTR(PAYLOAD_CONNECTED_TRUE), true);
      itoa(WiFi.RSSI(), strtmp, 10);
      MQTTclient.publish_P(MQTT_PREFIX TOPIC_RSSI, strtmp, true);
      WiFi.BSSIDstr().toCharArray(strtmp, 20);
      MQTTclient.publish_P(MQTT_PREFIX TOPIC_WIFI_BSSID, strtmp, true);     
      MQTTclient.subscribe(MQTT_SET_PREFIX "#");
    }
    else {
      Serial.print(F(" reconnect failed, reason "));
      itoa(MQTTclient.state(), strtmp, 10);
      Serial.print(strtmp);
      Serial.println(F(" try again in 5 seconds"));
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
      runtimeMillisMQTT = millis();
      MQTTclient.disconnect();
      while (millis() - runtimeMillisMQTT < 5000) { // Wait 5 seconds before retrying
        delay(0);
        ArduinoOTA.handle();
      }
    }
  }
}

int MQTTloop() {
  if(WiFi.status() != WL_CONNECTED)
    WIFI_lost++;

  if (!MQTTclient.connected()) {
    MQTT_lost++;
    MQTTreconnect();
    return 1;         // 1 means that it just reconnected
  }
  MQTTclient.loop();
  return 0;
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strcmp(OTA_PASSWORD, "") != 0)
    ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf_P(PSTR("Progress: %u%%\n"), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf_P(PSTR("Error[%u]: %i\n"), error);
    if (error == OTA_AUTH_ERROR)
      Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR)
      Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR)
      Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR)
      Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();
  Serial.println(F("OTA Ready"));
}
