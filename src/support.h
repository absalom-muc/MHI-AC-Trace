#ifndef SUPPORT_h
#define SUPPORT_h

#define VERSION "1.52"

#define WIFI_SSID "your SSID"
#define WIFI_PASSWORD "your WiFi password"
#define HOSTNAME "MHI-AC-Trace"

#define UseStrongestAP true                // when false then the first WiFi access point with matching SSID found is used.
                                           // when true then the strongest WiFi access point with matching SSID found is used, and
                                           // WiFi network re-scan every 12 minutes with alternate to +5dB stronger signal if detected

#define MQTT_SERVER "192.168.178.111"      // broker name or IP address of the broker
#define MQTT_PORT 1883                     // port number used by the broker
#define MQTT_USER ""                       // if authentication is not used, leave it empty
#define MQTT_PASSWORD ""                   // if authentication is not used, leave it empty
#define MQTT_PREFIX HOSTNAME "/"
#define MQTT_SET_PREFIX MQTT_PREFIX "set/" // prefix for subscribing set commands

#define OTA_HOSTNAME HOSTNAME              // default for the OTA_HOSTNAME is the HOSTNAME
#define OTA_PASSWORD ""                    // Enter an OTA password if required

#define MQTT_CHAR true                     // true  : MQTT raw as characters (lowest resources)
                                           // false : MQTT raw as hex values (human readable)

// *** The configuration ends here ***

#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>         // https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

extern PubSubClient MQTTclient;

void initWiFi();
void setupWiFi();
int MQTTloop();
void MQTTreconnect();
void setupOTA();

#define TOPIC_CONNECTED "connected"
#define TOPIC_VERSION "Version"
#define TOPIC_RSSI "RSSI"
#define TOPIC_WIFI_BSSID "WIFI_BSSID"

#define PAYLOAD_CONNECTED_TRUE "1"
#define PAYLOAD_CONNECTED_FALSE "0"

#define SCK_PIN  14
#define MOSI_PIN 13
#define MISO_PIN 12

#define SB0 0
#define SB1 SB0 + 1
#define SB2 SB0 + 2
#define DB0 SB2 + 1
#define DB1 SB2 + 2
#define DB2 SB2 + 3
#define DB3 SB2 + 4
#define DB4 SB2 + 5
#define DB6 SB2 + 7
#define DB9 SB2 + 10
#define DB11 SB2 + 12
#define DB12 SB2 + 13
#define DB13 SB2 + 14
#define DB14 SB2 + 15
#define CBH 18
#define CBL 19

#endif
