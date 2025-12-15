#include <Arduino.h>
#include <WiFi.h>
#include "Adafruit_GFX.h"
#include "Adafruit_I2CDevice.h"
#include "DotFlippersMatrix.h"
#include <SPIFFS.h>
#include "time.h"
#include "Fonts/Org_01.h"
#include <Muth01_4.h>

// constants
const char* ntpServer = "pool.ntp.org";
const char* ssid = "BoiteVivante";
const char* password = "82F7625EEC6894868EA329F8E1";


// global vars
struct tm timeinfo;
uint8_t time_digits[] = {0,0,0,0,0,0};
uint8_t animation[] = {
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,0,1,1,1,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,
1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0
};
DotFlippersMatrix dotFlippersMatrix = DotFlippersMatrix(288, 7);

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {

        case SYSTEM_EVENT_STA_START:
            WiFi.setHostname("FipDots");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("STA connected");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("STA got IP");
            Serial.print("STA SSID: ");
            Serial.println(WiFi.SSID());
            Serial.print("STA IPv4: ");
            Serial.println(WiFi.localIP());
            Serial.print("MAC address: ");
            Serial.println(WiFi.macAddress());

            configTime(3600, 3600, ntpServer, ntpServer, ntpServer);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.print("STA Disconnected. ");
            Serial.println(info.wifi_sta_disconnected.reason);
            WiFi.persistent(false);
            WiFi.disconnect(true);
            ESP.restart();
            break;
        default:
            break;
    }
}
void connectWiFi() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);

    WiFi.onEvent(WiFiEvent);
    WiFi.begin(ssid, password);
    Serial.println(F("WiFi connexion ongoing"));
}

// get time with NTP
void getTime() {
    if (!WiFi.isConnected()) {
        Serial.println("Not connected");
        return;
      }
    // get time
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    Serial.print(&timeinfo, "%A %d %B %Y %H:%M:%S");
    Serial.print(", RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.print(", Free Heap: ");
    Serial.println(ESP.getFreeHeap());

    // generate time numbers
    time_digits[5] = timeinfo.tm_sec % 10;
    time_digits[4] = timeinfo.tm_sec / 10;
    time_digits[3] = timeinfo.tm_min % 10;
    time_digits[2] = timeinfo.tm_min / 10;
    time_digits[1] = timeinfo.tm_hour % 10;
    time_digits[0] = timeinfo.tm_hour / 10;

}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  pinMode(15, INPUT_PULLUP);

  dotFlippersMatrix.begin();
  delay(1000); 

  dotFlippersMatrix.clear(0);
  dotFlippersMatrix.display();
  delay(1000);
  
//   connectWiFi();
//   delay(1000);

}

int count = 0;
uint8_t color = 1;

void loop() {
    
    // for(int x=0; x<288; x++){
    //     color = animation[(x+count)%288];
    //     dotFlippersMatrix.drawPixel(x, 0, color);
    //     color = animation[(x+count+1)%288];
    //     dotFlippersMatrix.drawPixel(x, 1, color);
    //     color = animation[(x+count+2)%288];
    //     dotFlippersMatrix.drawPixel(x, 2, color);
    //     color = animation[(x+count+3)%288];
    //     dotFlippersMatrix.drawPixel(x, 3, color);
    //     color = animation[(x+count+2)%288];
    //     dotFlippersMatrix.drawPixel(x, 4, color);
    //     color = animation[(x+count+1)%288];
    //     dotFlippersMatrix.drawPixel(x, 5, color);
    //     color = animation[(x+count)%288];
    //     dotFlippersMatrix.drawPixel(x, 6, color);
    // }
    // dotFlippersMatrix.display();
    // count += 1;
    // delay(20);

    dotFlippersMatrix.setXshift(0);
    dotFlippersMatrix.clear(1);
    dotFlippersMatrix.display();
    delay(10000);
    
    dotFlippersMatrix.clear(0);
    dotFlippersMatrix.display();
    delay(10000);

    // dotFlippersMatrix.setFont(&Org_01);
    dotFlippersMatrix.clear(0);
    dotFlippersMatrix.setCursor(0,0);
    dotFlippersMatrix.setTextColor(0xFF);
    dotFlippersMatrix.print("TESTING ... RING FLIP DOT DISPLAY ... 7 x 288");

    for(int x=0; x<288; x++){
        dotFlippersMatrix.setXshift(-x);
        dotFlippersMatrix.display();
        delay(40);
    }

    delay(10000);

}
