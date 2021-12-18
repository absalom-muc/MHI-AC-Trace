// MHI-AC-Trace by absalom-muc
// Trace of SPI signals for Mitsubishi Air Conditioner, print on serial debug console and publish via MQTT

#include "support.h"

bool sync = 0;
uint async_cnt = 0;

uint16_t packet_cnt = 0;
bool MOSI_diff = true;
bool MISO_diff = true;

void MQTT_subscribe_callback(char* topic, byte* payload, unsigned int length) {
  char payload_str[20];
  memcpy(payload_str, payload, length);
  payload_str[length] = '\0';

  if (strcmp(topic, MQTT_SET_PREFIX "reset") == 0) {
    if (strcmp(payload_str, "reset") == 0) {
      ESP.restart();
    }
  }
}

void update_sync(bool sync_new) {
  char strtmp[10]; // for the MQTT strings to send
  if (sync_new != sync) {
    sync = sync_new;
    //Serial.printf("synced=%i\n", sync);
    if (sync) {
      //Serial.println("sync=1");
      MQTTclient.publish (MQTT_PREFIX "synced", "1", true);
      itoa(async_cnt, strtmp, 10);
      MQTTclient.publish (MQTT_PREFIX "async_cnt", strtmp, true);
    }
    else {
      //Serial.println("synced=0");
      MQTTclient.publish (MQTT_PREFIX "synced", "0", true);
      async_cnt++;
      itoa(async_cnt, strtmp, 10);
      MQTTclient.publish (MQTT_PREFIX "async_cnt", strtmp, true);
      packet_cnt = 0;
      MOSI_diff = true;
      MISO_diff = true;
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("\n\n");
  Serial.println(F("Starting MHI-AC-Trace v" VERSION));
  Serial.printf_P(PSTR("CPU frequency[Hz]=%lu\n"), F_CPU);

  pinMode(SCK_PIN, INPUT_PULLUP);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);

  initWiFi();
  setupWiFi();
  setupOTA();
  MQTTclient.setServer(MQTT_SERVER, MQTT_PORT);
  MQTTclient.setCallback(MQTT_subscribe_callback);
  MQTTreconnect();
}

void loop() {
  char strtmp[100]; // for the MQTT strings to send
  byte MOSI_byte;
  byte MISO_byte;
  byte bit_mask;
  unsigned long lastDatapacketMillis = 0;
  unsigned long runtimeMillis = 0;
  unsigned long SCKMillis;
  //uint8_t MOSI_frame[20];
  uint8_t MISO_frame[20];
  bool valid_datapacket_received = false;
  uint8_t MQTTframe[43];
  byte repetitionNo = 0;

  for (uint8_t byte_cnt = 0; byte_cnt < 20; byte_cnt++) {
    //MOSI_frame[byte_cnt] = 0;
    MISO_frame[byte_cnt] = 0;
  }
  for (uint8_t byte_cnt = 0; byte_cnt < 43; byte_cnt++) {
    MQTTframe[byte_cnt] = 0;
  }
  while (1) {
    SCKMillis = millis();
    while (millis() - SCKMillis < 5) { // wait for 5ms stable high signal
      delay(0);
      if (!digitalRead(SCK)) {
        SCKMillis = millis();
        if (millis() - lastDatapacketMillis > 35)
          update_sync(false);
      }
    }

    uint16_t rx_checksum = 0;
    for (uint8_t byte_cnt = 0; byte_cnt < 20; byte_cnt++) { // read 1 data packet of 20 bytes
      MOSI_byte = 0;
      MISO_byte = 0;
      bit_mask = 1;
      for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++) { // read 1 byte
        while (digitalRead(SCK)) {} // wait for falling edge
        while (!digitalRead(SCK)) {} // wait for rising edge
        if (digitalRead(MOSI))
          MOSI_byte += bit_mask;    // read one MOSI byte
        if (digitalRead(MISO))
          MISO_byte += bit_mask;    // and the corresponding MISO byte
        bit_mask = bit_mask << 1;
      }

      if (byte_cnt < 18)
        rx_checksum += MOSI_byte;

      if (MQTTframe[2 + byte_cnt] != MOSI_byte) {
        MQTTframe[2 + byte_cnt] = MOSI_byte;
        // Ignore toggle of DB3 (room temp)
        if ((byte_cnt != DB3) & (byte_cnt != CBH) & (byte_cnt != CBL))
          MOSI_diff = true;
      }

      if (MISO_frame[byte_cnt] != MISO_byte) {
        MISO_frame[byte_cnt] = MISO_byte;
        MISO_diff = true;
      }
    }

    char buf[100];
    if ((MQTTframe[2+0] != 0x6c) | (MQTTframe[2+1] != 0x80) | (MQTTframe[2+2] != 0x04)) {
      String(packet_cnt).toCharArray(strtmp, 10);
      strcat(strtmp, " wrong MOSI signature ");
      String(MQTTframe[2+0], HEX).toCharArray(buf, 3);
      strcat(strtmp, buf);
      strcat(strtmp, " ");
      String(MQTTframe[2+1], HEX).toCharArray(buf, 3);
      strcat(strtmp, buf);
      strcat(strtmp, " ");
      String(MQTTframe[2+2], HEX).toCharArray(buf, 3);
      strcat(strtmp, buf);
      Serial.printf("%s\n", strtmp);
      MQTTclient.publish(MQTT_PREFIX "Error1", strtmp, true);
    }
    if ((MQTTframe[2+18] != highByte(rx_checksum)) | (MQTTframe[2+19] != lowByte(rx_checksum))) {
      String(packet_cnt).toCharArray(strtmp, 10);
      strcat(strtmp, " wrong MOSI checksum");
      Serial.printf("%s\n", strtmp);
      MQTTclient.publish(MQTT_PREFIX "Error2", strtmp, true);
    }
    if ((MQTTframe[2+0] == 0x6c) & (MQTTframe[2+1] == 0x80) & (MQTTframe[2+2] == 0x04) & (MQTTframe[2+18] == highByte(rx_checksum)) & (MQTTframe[2+19] == lowByte(rx_checksum)))
      valid_datapacket_received = true;
    else
      update_sync(false);

    if (valid_datapacket_received) { // valid frame received
      packet_cnt++;

      if (millis() - lastDatapacketMillis < 60)
        update_sync(true);
      lastDatapacketMillis = millis();

      if (MOSI_diff) {
        MOSI_diff = false;
        MQTTframe[0] = highByte(packet_cnt);
        MQTTframe[1] = lowByte(packet_cnt);
        MQTTframe[42] = repetitionNo;
        MQTTclient.publish(MQTT_PREFIX "raw", MQTTframe, 43, false);

        for (uint8_t byte_cnt = 0; byte_cnt < 43; byte_cnt++)
          Serial.printf("%02x ", MQTTframe[byte_cnt]);
        Serial.println();
        
        repetitionNo = 0;
      }
      else
        repetitionNo++;

      if (MISO_diff) {
        MISO_diff = false;
        for (uint8_t byte_cnt = 0; byte_cnt < 20; byte_cnt++) {
          MQTTframe[22 + byte_cnt] = MISO_frame[byte_cnt];  // wird hier evtl. der frame manchmal überschrieben, obwohl er noch nicht rausgeschrieben ist?? In der API keine Hinweise daruf geunden das Publish asynchron ist

          // siehe https://github.com/knolleary/pubsubclient/issues/452 und https://www.arduino.cc/en/Reference/ClientFlush        }
          MOSI_diff = true;
        }
      }
      valid_datapacket_received = false;
      digitalWrite(LED_BUILTIN, HIGH);
      if (!MQTTclient.connected())
        MQTTreconnect();
      MQTTclient.loop();
      ArduinoOTA.handle();
    }
  }
}
