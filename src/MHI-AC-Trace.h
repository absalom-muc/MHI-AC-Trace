#define SCK 14
#define MOSI 13
#define MISO 12

#define VERSION "1.4"

#define ssid  "your SSID"
#define password "your WiFi password"
#define HOSTNAME "MHI-AC-Trace"

#define MQTT_SERVER "192.168.178.111"      // MQTT broker name or IP address of the MQTT broker
#define MQTT_PORT 1883                     // port number used by the broker
#define MQTT_USER ""                       // if authentication is not used, leave it empty
#define MQTT_PASSWORD ""                   // if authentication is not used, leave it empty
#define MQTT_PREFIX HOSTNAME "/"
#define MQTT_SET_PREFIX MQTT_PREFIX "set/" // prefix for subscribing set commands


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
#define CBH 18
#define CBL 19

uint rising_edge_cnt_SCK = 0;
ICACHE_RAM_ATTR void handleInterrupt_SCK() {
  rising_edge_cnt_SCK++;
}

uint rising_edge_cnt_MOSI = 0;
ICACHE_RAM_ATTR void handleInterrupt_MOSI() {
  rising_edge_cnt_MOSI++;
}

uint rising_edge_cnt_MISO = 0;
ICACHE_RAM_ATTR void handleInterrupt_MISO() {
  rising_edge_cnt_MISO++;
}

#define cnt_MAX 4000
unsigned long falledge_time[cnt_MAX];
uint edge_cnt;
unsigned long start_micros;
bool acq_finish;
char fmsg[128];

// tick counter
// Quelle https://sub.nanona.fi/esp8266/timing-and-ticks.html
static inline uint32_t asm_ccount(void) {
    uint32_t r;
    asm volatile ("rsr %0, ccount" : "=r"(r));
    return r;
}

ICACHE_RAM_ATTR void handleInterrupt_SCK2() {
  if((uint)(micros()-start_micros)>=1000000)
   acq_finish = true;
  else
    falledge_time[edge_cnt++] = micros();//
}
void MeasureFrequency() {  // measure the frequency on the pins
  uint rising_edge_max_cnt_SCK = 0;
  uint rising_edge_total_cnt_SCK = 0;
  uint cnt = 0;
  rising_edge_cnt_SCK = 0;
  rising_edge_cnt_MOSI = 0;
  rising_edge_cnt_MISO = 0;  
  pinMode(SCK, INPUT);
  pinMode(MOSI, INPUT);
  pinMode(MISO, INPUT);
  Serial.println("Measure frequency for SCK, MOSI and MISO pin");
  //while(1){
  //f kann >150kHz bei 80MHz CPU Frequenz sein
  //f kann >300kHz bei 160MHz CPU Frequenz sein
  
  attachInterrupt(digitalPinToInterrupt(SCK), handleInterrupt_SCK, RISING);
  //attachInterrupt(digitalPinToInterrupt(MOSI), handleInterrupt_MOSI, RISING);
  //attachInterrupt(digitalPinToInterrupt(MISO), handleInterrupt_MISO, RISING);
  
  unsigned long starttimeMillis = millis();
  //while (millis() - starttimeMillis < 1000);
  delayMicroseconds(1000000);
  detachInterrupt(SCK);
  detachInterrupt(MOSI);
  detachInterrupt(MISO);
  if(rising_edge_cnt_SCK > rising_edge_max_cnt_SCK)
    rising_edge_max_cnt_SCK = rising_edge_cnt_SCK;
  rising_edge_total_cnt_SCK += rising_edge_cnt_SCK;
  cnt++;
  Serial.printf("SCK frequency=%iHz (expected: >3000Hz) max=%iHz med=%iHz ", rising_edge_cnt_SCK, rising_edge_max_cnt_SCK, rising_edge_total_cnt_SCK/cnt);
  if (rising_edge_cnt_SCK > 3000)
    Serial.println("o.k.");
  else
    Serial.println("out of range!");

  Serial.printf("MOSI frequency=%iHz (expected: <SCK frequency) ", rising_edge_cnt_MOSI);
  if (rising_edge_cnt_MOSI > 300 & rising_edge_cnt_MOSI < rising_edge_cnt_SCK)
    Serial.println("o.k.");
  else
    Serial.println("out of range!");

  Serial.printf("MISO frequency=%iHz (expected: ~0Hz) ", rising_edge_cnt_MISO);
  if (rising_edge_cnt_MISO >= 0 & rising_edge_cnt_MISO <= 10) {
    Serial.println("o.k.");
  }
  else {
    Serial.println("out of range!");
    //while (1);
  }
  /*rising_edge_cnt_SCK = 0;
  rising_edge_cnt_MOSI = 0;
  rising_edge_cnt_MISO = 0;
  delay(0);
  }*/

  delay(0);
  unsigned long cycle_time[3];
  unsigned long cycle_time_min[3];
  unsigned long cycle_time_max[3];
  unsigned long cycle_time_cnt[3];
  unsigned long diff;
  uint cy;


  Serial.println("");
  Serial.print("acquisition, please wait ... ");
  edge_cnt=0;
  acq_finish = false;
  start_micros = micros();
  attachInterrupt(digitalPinToInterrupt(SCK), handleInterrupt_SCK2, FALLING);
  while(!acq_finish) {
    delay(0);
  }
  detachInterrupt(SCK);
  Serial.println("ready");
  Serial.printf("start_micros=%luµs\n", start_micros);
  Serial.printf("start time=%luµs end time=%luµs\n", falledge_time[0], falledge_time[edge_cnt-1]);
  Serial.printf("acquiring time=%lums f=%i\n", (falledge_time[edge_cnt-1] - falledge_time[0])/1000, edge_cnt);

  //for(int i=0; i<20; i++)
  //  Serial.printf("falledge_time[%i]=%luµs, delta=%luµs\n", i, falledge_time[i], falledge_time[i]-falledge_time[i-1]);
  
  for(int cy=0; cy<3; cy++){
    cycle_time[cy]=0;
    cycle_time_min[cy]=999999;
    cycle_time_max[cy]=0;
    cycle_time_cnt[cy]=0;
  }
  for(int cnt=1; cnt<edge_cnt; cnt++){
    diff=falledge_time[cnt]-falledge_time[cnt-1];
    for(cy=0; cy<3; cy++){
      #define TOLERANCE 0.2f
      if((diff>=cycle_time[cy]*(1-TOLERANCE)) & (diff<=cycle_time[cy]*(1+TOLERANCE))){  // known range?
        if(diff < cycle_time_min[cy])
          cycle_time_min[cy] = diff;
        if(diff > cycle_time_max[cy])
          cycle_time_max[cy] = diff;
        cycle_time_cnt[cy]++;
        break;
      }
    }
    if(cy>=3){  // if not in range of used cycle_time
      for(cy=0; cy<3; cy++){  // find next unused cycle_time
        if(cycle_time[cy]==0){
          cycle_time[cy]=diff;
          cycle_time_min[cy]=diff;
          cycle_time_max[cy]=diff;
          cycle_time_cnt[cy]=1;
          break;
        }
      }
      if(cy>=3)
        break;
    }
  }
  
  if(cy>=3){
    Serial.println("more than 3 ranges detected, skip measurement");
    strcpy(fmsg, "more than 3 ranges detected, skip measurement");
  }
  else{
    for(cy=0; cy<3; cy++)
      Serial.printf("cycle_time_min[%i]=%luµs, delta_time[%i]=%luµs, cycle_time_cnt=%lu\n", cy, cycle_time_min[cy], cy, cycle_time_max[cy]-cycle_time_min[cy], cycle_time_cnt[cy]);

    unsigned long TBit;
    unsigned long TByte;
    unsigned long TBytePause;
    unsigned long TFrame;
    unsigned long TFramePause;
    unsigned long temp;
    
    if((cycle_time_min[0]<cycle_time_min[1]) & (cycle_time_min[1]<cycle_time_min[2])){
      TBit = cycle_time_min[0];
      TBytePause = cycle_time_min[1];
      TFramePause = cycle_time_min[2];
    }
    else if((cycle_time_min[0]<cycle_time_min[2]) & (cycle_time_min[2]<cycle_time_min[1])){
      TBit = cycle_time_min[0];
      TBytePause = cycle_time_min[2];
      TFramePause = cycle_time_min[1];
    }
    else if((cycle_time_min[1]<cycle_time_min[0]) & (cycle_time_min[0]<cycle_time_min[2])){
      TBit = cycle_time_min[1];
      TBytePause = cycle_time_min[0];
      TFramePause = cycle_time_min[2];
    }
    else if((cycle_time_min[1]<cycle_time_min[2]) & (cycle_time_min[2]<cycle_time_min[0])){
      TBit = cycle_time_min[1];
      TBytePause = cycle_time_min[2];
      TFramePause = cycle_time_min[0];
    }
    else if((cycle_time_min[2]<cycle_time_min[0]) & (cycle_time_min[0]<cycle_time_min[1])){
      TBit = cycle_time_min[2];
      TBytePause = cycle_time_min[0];
      TFramePause = cycle_time_min[1];
    }
    else if((cycle_time_min[2]<cycle_time_min[1]) & (cycle_time_min[1]<cycle_time_min[0])){
      TBit = cycle_time_min[2];
      TBytePause = cycle_time_min[1];
      TFramePause = cycle_time_min[0];
    }
    TBytePause -= TBit/2;
    TFramePause -= TBit/2;
    Serial.printf("TBit=%luµs, TBytePause=%luµs, TFramePause=%luµs\n", TBit, TBytePause, TFramePause);

    sprintf (fmsg, "TBit=%luµs, TBytePause=%luµs, TFramePause=%luµs", TBit, TBytePause, TFramePause);
  }
  delay(0);
}
