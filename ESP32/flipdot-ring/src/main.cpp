#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <sys/time.h>
#include <math.h>
#include "Adafruit_GFX.h"
#include "DotFlippersMatrix.h"
#include "time.h"
#include "config_page.h"
#include "driver/rtc_io.h"

// constants
const char* ntpServer = "pool.ntp.org";
static const char* PARIS_TZ = "CET-1CEST,M3.5.0/2,M10.5.0/3";
RTC_DATA_ATTR static int dayOfLastSinceSync = -1;

const uint16_t displayWidth = 288; // or 48 characters of 6 pixels width
const uint16_t displayHeight = 7;
const int CONFIG_PIN = D0; // GPIO pin to enter configuration mode 
const gpio_num_t CONFIG_GPIO = GPIO_NUM_0;
const int BATTERY_PIN = A1; // ADC pin for battery voltage measurement
const int LOW_BATTERY_THRESHOLD = 1650; // in millivolts

// Configuration structure
struct Config {
    char wifiSSID[32];
    char wifiPassword[64];
    char timezone[64];
    bool customConfiguration;   // flipdot boards will take a custom config in account.
    bool forceFlipping;         // force flipping even if the content is the same as previous frame
    bool driversPowerSaving;    // Switch off the flipdot drivers 5V power rail after latching
    uint8_t dotFlipTime;        // custom flip time for the dots in 100us unit, between 1 (100us) and 15 (1500us)
    bool initialized;
};

// global vars
struct tm timeinfo;
DotFlippersMatrix flipdotMatrix = DotFlippersMatrix(displayWidth, displayHeight);
Preferences preferences;
Config config;
WebServer server(80);
int mainBatteryMiliVolts;

// Load configuration from preferences (or set defaults if not found)
void loadConfig() {
    preferences.begin("clock-config", true); // read-only
    config.initialized = preferences.getBool("initialized", false);
    
    if (config.initialized) {
        preferences.getString("ssid", config.wifiSSID, sizeof(config.wifiSSID));
        preferences.getString("password", config.wifiPassword, sizeof(config.wifiPassword));
        preferences.getString("timezone", config.timezone, sizeof(config.timezone));
        config.customConfiguration = preferences.getBool("customConfig", true);
        config.forceFlipping = preferences.getBool("forceFlip", true);
        config.driversPowerSaving = preferences.getBool("FPpowSave", false);
        config.dotFlipTime = preferences.getUChar("dotFlipTime", 4);
        Serial.println("Configuration loaded from storage");
    } else {
        // Set defaults
        strcpy(config.wifiSSID, "");
        strcpy(config.wifiPassword, "");
        strcpy(config.timezone, PARIS_TZ);
        config.customConfiguration = true;
        config.forceFlipping = true;
        config.driversPowerSaving = false;
        config.dotFlipTime = 4; // default to 400us
        Serial.println("No configuration found, using defaults");
    }
    
    preferences.end();
}

// Save configuration to preferences
void saveConfig() {
    preferences.begin("clock-config", false); // read-write
    preferences.putBool("initialized", true);
    preferences.putString("ssid", config.wifiSSID);
    preferences.putString("password", config.wifiPassword);
    preferences.putString("timezone", config.timezone);
    preferences.putBool("customConfig", config.customConfiguration);
    preferences.putBool("forceFlip", config.forceFlipping);
    preferences.putBool("FPpowSave", config.driversPowerSaving);
    preferences.putUChar("dotFlipTime", config.dotFlipTime);
    preferences.end();
    config.initialized = true;
    Serial.println("Configuration saved to storage");
}

// Escape HTML special characters in a string
String escapeHtml(const char *input) {
    String out;
    if (!input) {
        return out;
    }

    for (const char *p = input; *p; ++p) {
        switch (*p) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += *p; break;
        }
    }

    return out;
}

// Build the HTML configuration page with current settings
String buildConfigPageHtml() {
    String page = FPSTR(htmlPage);

    page.replace("name=\"ssid\" required placeholder=\"Enter WiFi network name\"",
                 "name=\"ssid\" required value=\"" + escapeHtml(config.wifiSSID) + "\" placeholder=\"Enter WiFi network name\"");
    page.replace("name=\"password\" placeholder=\"Enter WiFi password\"",
                 "name=\"password\" value=\"" + escapeHtml(config.wifiPassword) + "\" placeholder=\"Enter WiFi password\"");
    page.replace("name=\"timezone\" value=\"CET-1CEST,M3.5.0/2,M10.5.0/3\"",
                 "name=\"timezone\" value=\"" + escapeHtml(config.timezone) + "\"");
    page.replace("name=\"dotFlipTime\" value=\"4\"",
                 "name=\"dotFlipTime\" value=\"" + String(config.dotFlipTime) + "\"");

    page.replace("id=\"customConfig\" name=\"customConfig\" value=\"1\" checked",
                 String("id=\"customConfig\" name=\"customConfig\" value=\"1\"") + (config.customConfiguration ? " checked" : ""));
    page.replace("id=\"forceFlip\" name=\"forceFlip\" value=\"1\" checked",
                 String("id=\"forceFlip\" name=\"forceFlip\" value=\"1\"") + (config.forceFlipping ? " checked" : ""));
    page.replace("id=\"driversPowerSave\" name=\"driversPowerSave\" value=\"1\" checked",
                 String("id=\"driversPowerSave\" name=\"driversPowerSave\" value=\"1\"") + (config.driversPowerSaving ? " checked" : ""));

    return page;
}

// Web server handler for the root page
void handleRoot() {
    server.send(200, "text/html", buildConfigPageHtml());
}

// Web server handler for saving configuration
void handleSave() {
    if (server.hasArg("ssid")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        String timezone = server.arg("timezone");
        
        ssid.toCharArray(config.wifiSSID, sizeof(config.wifiSSID));
        password.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
        timezone.toCharArray(config.timezone, sizeof(config.timezone));
        
        // Get flipdot display settings
        config.customConfiguration = server.hasArg("customConfig");
        config.forceFlipping = server.hasArg("forceFlip");
        config.driversPowerSaving = server.hasArg("driversPowerSave");
        
        if (server.hasArg("dotFlipTime")) {
            int flipTime = server.arg("dotFlipTime").toInt();
            config.dotFlipTime = constrain(flipTime, 1, 15); // constrain to valid range
        } else {
            config.dotFlipTime = 4; // default
        }
        
        saveConfig();
        
        String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'><style>body{font-family:Arial;text-align:center;padding:50px;background:#f0f0f0;}";
        response += ".message{background:white;padding:30px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);max-width:400px;margin:0 auto;}";
        response += "h1{color:#4CAF50;}</style></head><body><div class='message'><h1>✓ Configuration Saved!</h1>";
        response += "<p>Device will restart in 3 seconds...</p></div></body></html>";
        
        server.send(200, "text/html", response);
        delay(3000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

// Web server handler for setting time manually, used when no WiFi connection is available.
void handleSetTime() {
    if (!server.hasArg("setHour") || !server.hasArg("setMinute") || !server.hasArg("setDay") || !server.hasArg("setMonth") || !server.hasArg("setYear")) {
        server.send(400, "text/plain", "Missing date/time parameters");
        return;
    }

    int setHour = server.arg("setHour").toInt();
    int setMinute = server.arg("setMinute").toInt();
    int setDay = server.arg("setDay").toInt();
    int setMonth = server.arg("setMonth").toInt();
    int setYear = server.arg("setYear").toInt();

    if (setHour < 0 || setHour > 23 || setMinute < 0 || setMinute > 59 || setDay < 1 || setDay > 31 || setMonth < 1 || setMonth > 12 || setYear < 2000 || setYear > 2099) {
        server.send(400, "text/plain", "Invalid date/time values");
        return;
    }

    setenv("TZ", config.timezone, 1);
    tzset();

    struct tm newTime = {};
    newTime.tm_year = setYear - 1900;
    newTime.tm_mon = setMonth - 1;
    newTime.tm_mday = setDay;
    newTime.tm_hour = setHour;
    newTime.tm_min = setMinute;
    newTime.tm_sec = 0;

    time_t epoch = mktime(&newTime);
    if (epoch == (time_t)-1) {
        server.send(400, "text/plain", "Failed to build time from provided values");
        return;
    }

    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    if (settimeofday(&tv, nullptr) != 0) {
        server.send(500, "text/plain", "Failed to set RTC time");
        return;
    }

    struct tm verifyTime;
    getLocalTime(&verifyTime);
    dayOfLastSinceSync = verifyTime.tm_mday;

    String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'><style>body{font-family:Arial;text-align:center;padding:50px;background:#f0f0f0;}";
    response += ".message{background:white;padding:30px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);max-width:500px;margin:0 auto;}";
    response += "h1{color:#4CAF50;}a{display:inline-block;margin-top:15px;color:#2196F3;text-decoration:none;}</style></head><body><div class='message'><h1>✓ Time Set</h1>";
    response += "<p>RTC updated to: " + String(setYear) + "-" + String(setMonth) + "-" + String(setDay) + " " + String(setHour) + ":" + String(setMinute) + "</p>";
    response += "<a href='/'>Back to configuration</a></div></body></html>";

    server.send(200, "text/html", response);

    delay(3000);
    ESP.restart();
}

// Start configuration mode (Access Point + Web Server)
void startConfigMode() {
    Serial.println("Starting configuration mode.");
    
    // Create Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP("FlipClock", "12345678");
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.println("Connect to WiFi 'FlipClock' with password '12345678'");
    Serial.println("Then open http://192.168.4.1 in your browser");
    
    // Display setup info on flipdot display
    flipdotMatrix.clear(0);
    flipdotMatrix.display(); 
    flipdotMatrix.setCursor(0, 0);
    flipdotMatrix.print("WiFi FlipClock:12345678 http://192.168.4.1 ");
    flipdotMatrix.display();
    
    // Initialize web server
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/settime", HTTP_POST, handleSetTime);
    server.begin();
    
    Serial.println("Web server started");
    
    // Keep server running until config button is pressed again
    while (true) {
        server.handleClient();
        delay(10);
        if (digitalRead(CONFIG_PIN) == LOW) { 
            break;
        }
    }
}

// WiFi event handler
void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {

        case WIFI_EVENT_STA_START:
            WiFi.setHostname("STA started");
            Serial.print("MAC address: ");
            Serial.println(WiFi.macAddress());
            break;
        case WIFI_EVENT_STA_CONNECTED:
            Serial.println("STA connected");
            break;
        case IP_EVENT_STA_GOT_IP:
            Serial.println("STA got IP");
            Serial.print("STA SSID: ");
            Serial.println(WiFi.SSID());
            Serial.print("STA IPv4: ");
            Serial.println(WiFi.localIP());
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
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
    
    // Use stored configuration
    if (strlen(config.wifiSSID) > 0) {
        Serial.print("Connecting to: ");
        Serial.println(config.wifiSSID);
        WiFi.begin(config.wifiSSID, config.wifiPassword);
    } else {
        Serial.println("No WiFi SSID configured!");
        return;
    }
    
    // Wait for connection with a timeout of 5 seconds
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 5000) {
        delay(500);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("WiFi connected"));
    } else {
        Serial.println(F("WiFi connection failed"));
    }
}

// disable WiFi completely
void disableWiFi() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disabled");
}

// go into deep sleep for 1 minute minus negativeDelayS seconds
void goToDeepSleep1min(int negativeDelayS = 0) {
    Serial.print("Going to deep sleep for ");
    Serial.print(60 - negativeDelayS);
    Serial.println(" seconds");
    Serial.flush(); 
    Serial.end();
    delay(100);
    esp_sleep_enable_timer_wakeup((60 - negativeDelayS) * 1000000ULL);
    esp_deep_sleep_start();
}

// go into deep sleep for 10 minutes
void goToDeepSleep10min() {
    Serial.println("Going to deep sleep for 10min ");
    Serial.flush(); 
    Serial.end();
    delay(100);
    esp_sleep_enable_timer_wakeup(600 * 1000000ULL);
    esp_deep_sleep_start();
}

// confiture timezone for NTP
void configTimeZone() {
    configTime(3600, 3600, ntpServer, ntpServer, ntpServer);
    setenv("TZ", config.timezone, 1);
    tzset();
}

// print the reason for wake up, used for debug
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup caused by : %d\n",wakeup_reason); break;
  }
}

void configureFlipdotforLowPower() {
    flipdotMatrix.setCustomConfiguration(config.customConfiguration);
    flipdotMatrix.setForceFlipping(config.forceFlipping);
    flipdotMatrix.setDriversPowerSaving(config.driversPowerSaving);
    flipdotMatrix.setDotFlipTime(config.dotFlipTime); 
    flipdotMatrix.setXshift(0);
    flipdotMatrix.setTextColor(0xFF);
    flipdotMatrix.setTextWrap(false); 
}

void configureFlipdotForDemo() {
    flipdotMatrix.setCustomConfiguration(true);
    flipdotMatrix.setForceFlipping(false);
    flipdotMatrix.setDriversPowerSaving(false);
    flipdotMatrix.setDotFlipTime(2); 
    flipdotMatrix.setXshift(0);
    flipdotMatrix.setTextColor(0xFF);
    flipdotMatrix.setTextWrap(false); 
}

// Spinner animation demo
void demoMode() {
    const uint8_t minBarY = 0;
    const uint8_t maxBarY = 6;
    const uint8_t minBarHeight = 1;
    const uint8_t maxBarHeight = 7;
    const uint16_t minBarWidth = 10;
    const uint16_t maxExtraBarWidth = 48;
    const float rotationPeriodMs = 2100.0f; 
    const float pulsePeriodMs = 1400.0f; 
    const uint32_t frameDelayMs = 50;      

    configureFlipdotForDemo();

    uint32_t animationStartMs = millis();

    do {
        uint32_t elapsedMs = millis() - animationStartMs;
        float rotationPhase = (static_cast<float>(elapsedMs % static_cast<uint32_t>(rotationPeriodMs)) / rotationPeriodMs) * 2.0f * PI;
        float pulsePhase = (static_cast<float>(elapsedMs % static_cast<uint32_t>(pulsePeriodMs)) / pulsePeriodMs) * 2.0f * PI;

        // Bar width: sharp pulse, more like a "tail"
        float widthPulse = powf((sinf(pulsePhase) + 1.0f) * 0.5f, 1.7f); // sharper peak
        uint16_t barWidth = minBarWidth + static_cast<uint16_t>(widthPulse * maxExtraBarWidth);

        // Bar thickness: also varies, but less pronounced
        float thicknessPulse = powf((sinf(pulsePhase + 1.2f) + 1.0f) * 0.5f, 1.3f); // phase offset for variety
        uint8_t barHeight = minBarHeight + static_cast<uint8_t>(thicknessPulse * (maxBarHeight - minBarHeight));

        // Center the bar vertically
        uint8_t barY = minBarY + ((maxBarY - minBarY) / 2) - (barHeight / 2);

        uint16_t barX = static_cast<uint16_t>(((rotationPhase / (2.0f * PI)) * displayWidth));

        flipdotMatrix.clear(0);
        flipdotMatrix.fillRect(barX, barY, barWidth, barHeight, 0xFF);
        flipdotMatrix.display();

        delay(frameDelayMs);

    } while (digitalRead(CONFIG_PIN) != LOW); // stay in demo mode until config button is pressed
}

// Compute the image to display on the flipdot based on the current time
// the minute digits are at hour hand position.
void display_time() {
    uint16_t xMinutePosition = 0;
    char buf[4];
    uint16_t xBox = 0;
    uint16_t boxWidth = 0;

    configureFlipdotforLowPower();
    
    // calculate x position of minutes chars start point, based on the hour and minute
    xMinutePosition = ((((timeinfo.tm_hour%12)/12.0)*(displayWidth)) + ((timeinfo.tm_min/60.0)*(14.0)));

    // draw a thick line starting or ending at 0, up to the position of the minutes, to ease the hours reading.
    if ( (timeinfo.tm_hour >= 12 && timeinfo.tm_hour < 24)) { // box following the digits in the afternoon
        if (xMinutePosition < displayWidth-14) { // only if there is space to draw the box
            xBox = xMinutePosition+14;
            boxWidth = displayWidth - xBox;
            flipdotMatrix.fillRect(xBox, 2, boxWidth, 3, 0xFF);
        }
        if ( xMinutePosition <= 3) {  // we need to clear the space before the digits when the hour is 12.
            flipdotMatrix.fillRect(displayWidth - (3-xMinutePosition), 2, (3-xMinutePosition), 3, 0x00);
        }
    } else { // box preceding the digits in the morning
        if (xMinutePosition > 3 ) { // only if there is space to draw the box
            xBox = 0;
            boxWidth = xMinutePosition-3;
            flipdotMatrix.fillRect(xBox, 2, boxWidth, 3, 0xFF);
        }
        if ( xMinutePosition >= displayWidth-14) {  // we need to clear the space after the digits when the hour is 23.
            flipdotMatrix.fillRect(0, 2, 3-(displayWidth - (xMinutePosition+14)), 3, 0x00);
        }
    }

    // minutes in a char array
    sniprintf(buf, 4, "%02d", timeinfo.tm_min);

    // if the hour is between 3 and 9, draw minutes chars upside down, otherwise draw normally
    if (timeinfo.tm_hour%12 >=3 && timeinfo.tm_hour%12 < 9) {
        flipdotMatrix.drawCharsUpSideDown(xMinutePosition, 0, buf, 0xFF);
    } else {
        flipdotMatrix.setTextColor(0xFF);
        flipdotMatrix.setCursor(xMinutePosition, 0);
        flipdotMatrix.print(String(buf));
    }

    // refresh flipdot display
    flipdotMatrix.display();
}

// Display a low battery message on the flipdot
void display_LowBatteryMessage() {
    configureFlipdotforLowPower();
    flipdotMatrix.clear(0);
    flipdotMatrix.display(); 
    flipdotMatrix.setCursor(0, 0);
    flipdotMatrix.print("Low battery. ( " + String(mainBatteryMiliVolts) + "mV )");
    flipdotMatrix.display();

}

// we start here.
void setup() {
    // Setup the configuration pin
    pinMode(CONFIG_GPIO, INPUT_PULLUP);
    // enable wake up from the configuration pin
    rtc_gpio_init(CONFIG_GPIO); // needed for the pin to work as wakeup source
    rtc_gpio_set_direction(CONFIG_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(CONFIG_GPIO); // disable pull-down resistor for the config pin
    rtc_gpio_pullup_en(CONFIG_GPIO); // enable pull-up resistor for the config pin
    esp_sleep_enable_ext1_wakeup(1ULL << CONFIG_GPIO, ESP_EXT1_WAKEUP_ANY_LOW);

    //setup built-in LED pin as output
    pinMode(LED_BUILTIN, OUTPUT);
    // turn ON built-in LED to indicate the device is awake
    digitalWrite(LED_BUILTIN, LOW); 
    
    // initialize serial, it is native USB serial so it will only work if a PC is connected.
    Serial.begin(115200);
    
    // ADC: Set the resolution to 12 bits (0-4095)
    analogReadResolution(12);
    // read battery voltage in millivolts
    mainBatteryMiliVolts = analogReadMilliVolts(BATTERY_PIN);
    Serial.print("Battery voltage (mV): ");
    Serial.println(mainBatteryMiliVolts);

    // for debug
    // print_wakeup_reason();
    
    // Load configuration from storage
    loadConfig();
    
    // Start flipdot display instance
    flipdotMatrix.begin();
    
    // if battery is low, sleep for 10min and display a message.
    if (mainBatteryMiliVolts < LOW_BATTERY_THRESHOLD) { 
        display_LowBatteryMessage();
        goToDeepSleep10min();
    }

    // Check if configuration button is pressed OR configuration is not initialized
    if (digitalRead(CONFIG_PIN) == LOW || !config.initialized || strlen(config.wifiSSID) == 0) {
        if (digitalRead(CONFIG_PIN) == LOW) {
            Serial.println("Configuration button pressed - entering setup mode");
        } else {
            Serial.println("No WiFi configuration found - entering setup mode");
        }

        startConfigMode(); // if we exit this function, it means we pressed the config button.
        demoMode(); // So we start the demo mode, will return if config button is pressed again.
    }

    // configure timezone otherwise we may get wrong time from RTC which is not in UTC
    configTimeZone(); 
    // get time from RTC
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time 1");
    }

    Serial.println("Previous wifi sync day: " + String(dayOfLastSinceSync));

    // check if we need to sync time with NTP server (we do it once a day at 12 to save battery)
    if (dayOfLastSinceSync == -1 || (timeinfo.tm_hour == 12 && timeinfo.tm_mday != dayOfLastSinceSync)) {
        Serial.println("Connect to Wifi and sync time with NTP server. ");
        connectWiFi();
        delay(500);
        configTimeZone();
        delay(500);
        if (getLocalTime(&timeinfo))  dayOfLastSinceSync = timeinfo.tm_mday;
        else Serial.println("Failed to obtain time 2");
    } else {
        Serial.println("Time already synced today, no need to connect to WiFi.");
    }
    
    // compute and display the time on the flipdot
    display_time();

    Serial.println(&timeinfo, "%A %d %B %Y %H:%M:%S");

    disableWiFi();

    // we can reduce the deep sleep time to wake up closer to the next minute
    int compensationDelayS = 0;
    if (timeinfo.tm_sec > 3) compensationDelayS = 2; 
    if (timeinfo.tm_sec > 25) compensationDelayS = 9; 

    // turn off built-in LED to indicate the device is going to sleep
    digitalWrite(LED_BUILTIN, HIGH); 

    // go to deep sleep until the next minute (minus the compensation delay)
    // if Serial is not null, it means a PC is connected, so we stay awake for debugging purposes
    if (!Serial)  {
        if (timeinfo.tm_hour > 22 || timeinfo.tm_hour < 6) {  // if it's night time, sleep for 10min.
            goToDeepSleep10min();
        } else {
            goToDeepSleep1min(compensationDelayS);
        }
    }
}
void loop() {
    Serial.println("Debugging loop. restarting in 30 seconds.");
    delay(30000);
    ESP.restart();
}