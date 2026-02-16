#include <Arduino.h>
#include <WiFi.h>
#include "Adafruit_GFX.h"
#include "DotFlippersMatrix.h"
#include "time.h"

// constants
const char* ntpServer = "pool.ntp.org";
const char* ssid = "BoiteVivante";
const char* password = "82F7625EEC6894868EA329F8E1";
static const char* PARIS_TZ = "CET-1CEST,M3.5.0/2,M10.5.0/3";
RTC_DATA_ATTR static int hourOfLastSinceSync = -1;

const uint16_t displayWidth = 288;
const uint16_t displayHeight = 7;

// global vars
struct tm timeinfo;
DotFlippersMatrix dotFlippersMatrix = DotFlippersMatrix(displayWidth, displayHeight);

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {

        case SYSTEM_EVENT_STA_START:
            WiFi.setHostname("STA started");
            Serial.print("MAC address: ");
            Serial.println(WiFi.macAddress());
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
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.print("STA Disconnected, reason: ");
            Serial.println(info.wifi_sta_disconnected.reason);
            WiFi.persistent(false);
            WiFi.disconnect(true);
            break;
        default:
            break;
    }
}


// connect to wifi
void connectWiFi() {
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(WiFiEvent);
    WiFi.begin("CERN");
    // WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
        delay(500);
    }
    Serial.println(F("WiFi connexion"));
}

//disable WiFi completely
void disableWiFi() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disabled");
}

// go into deep sleep for 1 minute
void goToDeepSleep() {
    Serial.println("Going to deep sleep for 1 minute...");
    esp_sleep_enable_timer_wakeup(1 * 60 * 1000000);
    esp_deep_sleep_start();
}

// confiture timezone for NTP
void configTimeZone() {
    configTime(3600, 3600, ntpServer, ntpServer, ntpServer);
    setenv("TZ", PARIS_TZ, 1);
    tzset();
}

void setup() {
    // initialize serial and wait for it to be ready
    Serial.begin(115200);
    while (!Serial) { ; }
    pinMode(15, INPUT_PULLUP);

    // get time from RTC
    configTimeZone(); // configure timezone otherwise we may get wrong time from RTC, which is not in UTC
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
    }

    // check if we need to sync time with NTP server (we do it once per hour to save battery)
    if (hourOfLastSinceSync == -1 || hourOfLastSinceSync != timeinfo.tm_hour) {
        Serial.println("Connect to Wifi and sync time with NTP server. ");
        connectWiFi();
        delay(1000);
        configTimeZone();
        if (getLocalTime(&timeinfo))  hourOfLastSinceSync = timeinfo.tm_hour;
        else Serial.println("Failed to obtain time.");
    } else {
        Serial.println("Time is already synced less than an hour ago, no need to connect to WiFi.");
    }
    
    dotFlippersMatrix.begin();
    dotFlippersMatrix.setCustomConfiguration(true); // custom config
    dotFlippersMatrix.setForceFlipping(true);
    dotFlippersMatrix.setDotFlipTime(6); // set flipping time 
    dotFlippersMatrix.setXshift(0);
    dotFlippersMatrix.setTextColor(0xFF);

    uint16_t xMinutePosition = 0;
    char buf[4];
    
    // minutes in a char array
    sniprintf(buf, 4, "%02d", timeinfo.tm_min);
    
    // calculate x position of minutes, based on the hour and minute
    xMinutePosition = ((((timeinfo.tm_hour%12)/12.0)*(displayWidth)) + ((timeinfo.tm_min/60.0)*(15.0)));

    // if the hour is between 3 and 9, draw minutes upside down, otherwise draw normally
    if (timeinfo.tm_hour%12 >=3 && timeinfo.tm_hour%12 < 9) {
        dotFlippersMatrix.drawCharsUpSideDown(xMinutePosition, 0, buf, 0xFF);
    } else {
        dotFlippersMatrix.setCursor(xMinutePosition, 0);
        dotFlippersMatrix.print(timeinfo.tm_min);
    }

    // draw a box starting or ending at 0 up to the position of the minutes, to represent the hours
    if (timeinfo.tm_hour >= 12 && timeinfo.tm_hour < 24) {
        if (xMinutePosition < displayWidth-14) {
            dotFlippersMatrix.fillRect(xMinutePosition+14, 2, displayWidth - (xMinutePosition+14), 3, 0xFF);
        }
    } else {
        if (xMinutePosition > 3) {
            dotFlippersMatrix.fillRect(0, 2, xMinutePosition-3, 3, 0xFF);
        }
    }

    // refresh flipdot display
    dotFlippersMatrix.display();

    Serial.println(&timeinfo, "%A %d %B %Y %H:%M:%S");
    Serial.println("previous sync hour: " + String(hourOfLastSinceSync));

    disableWiFi();
    delay(200); // to flush serial buffer 
    goToDeepSleep();
}

void loop() {

    // dotFlippersMatrix.clear(0);
    // dotFlippersMatrix.display();
    // delay(1000);

    // dotFlippersMatrix.clear(0);
    // dotFlippersMatrix.setCursor(0,0);
    // dotFlippersMatrix.setTextColor(0xFF);
    // dotFlippersMatrix.drawCharsUpSideDown(0, 0, "A7", 0xFF);
    // dotFlippersMatrix.setCursor(12,0);
    // dotFlippersMatrix.print("1M");
    // dotFlippersMatrix.display();
    // delay(10000);

}