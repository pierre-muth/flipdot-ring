#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <sys/time.h>
#include "Adafruit_GFX.h"
#include "DotFlippersMatrix.h"
#include "time.h"

// constants
const char* ntpServer = "pool.ntp.org";
static const char* PARIS_TZ = "CET-1CEST,M3.5.0/2,M10.5.0/3";
RTC_DATA_ATTR static int hourOfLastSinceSync = -1;

const uint16_t displayWidth = 288;
const uint16_t displayHeight = 7;
const int CONFIG_PIN = 15;

// Configuration structure
struct Config {
    char wifiSSID[32];
    char wifiPassword[64];
    char timezone[64];
    bool customConfiguration;
    bool forceFlipping;
    bool driversPowerSaving;
    uint8_t dotFlipTime;
    bool initialized;
};

// global vars
struct tm timeinfo;
DotFlippersMatrix dotFlippersMatrix = DotFlippersMatrix(displayWidth, displayHeight);
Preferences preferences;
Config config;
WebServer server(80);

// Load configuration from preferences
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

// HTML configuration page
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Clock Configuration</title>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; }
        h2 { color: #555; font-size: 18px; margin-top: 25px; border-bottom: 2px solid #4CAF50; padding-bottom: 5px; }
        label { display: block; margin-top: 15px; font-weight: bold; color: #555; }
        input[type="text"], input[type="password"], input[type="number"] { width: 100%; padding: 8px; margin-top: 5px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        .checkbox-group { margin-top: 15px; }
        .checkbox-group label { display: inline-block; font-weight: normal; margin-left: 8px; }
        .checkbox-group input[type="checkbox"] { width: auto; }
        button { width: 100%; padding: 12px; margin-top: 20px; background: #4CAF50; color: white; border: none; border-radius: 4px; font-size: 16px; cursor: pointer; }
        button:hover { background: #45a049; }
        .info { background: #e7f3fe; padding: 10px; border-left: 4px solid #2196F3; margin-bottom: 20px; font-size: 14px; }
        .help { font-size: 12px; color: #888; margin-top: 3px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>⏰ Flip Clock Configuration</h1>
        <div class="info">Configure your WiFi, timezone, and flipdot display settings.</div>
        <form action="/save" method="POST">
            <h2>📡 WiFi Settings</h2>
            <label>WiFi SSID:</label>
            <input type="text" name="ssid" required placeholder="Enter WiFi network name">
            
            <label>WiFi Password:</label>
            <input type="password" name="password" placeholder="Enter WiFi password">
            
            <label>Timezone (POSIX format):</label>
            <input type="text" name="timezone" value="CET-1CEST,M3.5.0/2,M10.5.0/3" placeholder="e.g., CET-1CEST,M3.5.0/2,M10.5.0/3">
            <div class="help">Examples: CET-1CEST,M3.5.0/2,M10.5.0/3 (Europe Paris) | PST8PDT,M3.2.0,M11.1.0 (US Pacific)</div>
            
            <h2>🔢 Flipdot Display Settings</h2>
            <div class="checkbox-group">
                <input type="checkbox" id="customConfig" name="customConfig" value="1" checked>
                <label for="customConfig">Custom Configuration</label>
            </div>
            <div class="help">Enable custom display configuration</div>
            
            <div class="checkbox-group">
                <input type="checkbox" id="forceFlip" name="forceFlip" value="1" checked>
                <label for="forceFlip">Force Flipping</label>
            </div>
            <div class="help">Force dots flipping even if no change</div>
            
            <div class="checkbox-group">
                <input type="checkbox" id="driversPowerSave" name="driversPowerSave" value="1" checked>
                <label for="driversPowerSave">Drivers Power Saving</label>
            </div>
            <div class="help">Switching drivers 5V off between display refreshes</div>
            
            <label>Dot Flip Time (in 100s of us):</label>
            <input type="number" name="dotFlipTime" value="4" min="1" max="15" placeholder="4">
            <div class="help">Time in 100s of microseconds for each dot to flip (1-15, default: 4)</div>
            
            <button type="submit">Save Configuration</button>
        </form>

        <form action="/settime" method="POST">
            <h2>🕒 Manual Time Setup</h2>
            <div class="help">Use this if WiFi/NTP is unavailable. This sets the ESP32 RTC time directly.</div>

            <label>Hour (0-23):</label>
            <input type="number" name="setHour" value="12" min="0" max="23" required>

            <label>Minute (0-59):</label>
            <input type="number" name="setMinute" value="0" min="0" max="59" required>

            <label>Day (1-31):</label>
            <input type="number" name="setDay" value="1" min="1" max="31" required>

            <label>Month (1-12):</label>
            <input type="number" name="setMonth" value="1" min="1" max="12" required>

            <label>Year (e.g. 2026):</label>
            <input type="number" name="setYear" value="2026" min="2000" max="2099" required>

            <button type="submit">Set Time</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

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

// Web server handlers
void handleRoot() {
    server.send(200, "text/html", buildConfigPageHtml());
}

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
    hourOfLastSinceSync = verifyTime.tm_hour;

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
    Serial.println("Starting configuration mode...");
    
    // Create Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP("FlipClock", "12345678");
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.println("Connect to WiFi 'FlipClock' with password '12345678'");
    Serial.println("Then open http://192.168.4.1 in your browser");
    
    // Display setup info on flipdot display
    dotFlippersMatrix.clear(0);
    dotFlippersMatrix.display(); 
    dotFlippersMatrix.setCursor(0, 0);
    dotFlippersMatrix.print("WiFi FlipClock:12345678  http://192.168.4.1");
    dotFlippersMatrix.display();
    
    // Initialize web server
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/settime", HTTP_POST, handleSetTime);
    server.begin();
    
    Serial.println("Web server started");
    
    // Keep server running
    while (true) {
        server.handleClient();
        delay(10);
    }
}

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

//disable WiFi completely
void disableWiFi() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disabled");
}

// go into deep sleep for 1 minute minus negativeDelayS seconds
void goToDeepSleep1m(int negativeDelayS = 0) {
    Serial.print("Going to deep sleep for ");
    Serial.print(60 - negativeDelayS);
    Serial.println(" seconds");
    delay(100); // to flush serial buffer
    esp_sleep_enable_timer_wakeup((60 - negativeDelayS) * 1000000);
    esp_deep_sleep_start();
}

// confiture timezone for NTP
void configTimeZone() {
    configTime(3600, 3600, ntpServer, ntpServer, ntpServer);
    setenv("TZ", config.timezone, 1);
    tzset();
}

void setup() {
    // initialize serial and wait for it to be ready
    Serial.begin(115200);
    while (!Serial) { ; }

    // Setup configuration pin and load configuration
    pinMode(CONFIG_PIN, INPUT_PULLUP);
    loadConfig();
    
    // initialize flipdot display
    dotFlippersMatrix.begin();
    dotFlippersMatrix.setCustomConfiguration(config.customConfiguration); // use configured setting
    dotFlippersMatrix.setForceFlipping(config.forceFlipping);
    dotFlippersMatrix.setDriversPowerSaving(config.driversPowerSaving);
    dotFlippersMatrix.setDotFlipTime(config.dotFlipTime); // use configured flip time 
    dotFlippersMatrix.setXshift(0);
    dotFlippersMatrix.setTextColor(0xFF);
    dotFlippersMatrix.setTextWrap(false); // does it solve the issue of the second minute char not printed when it is 11:59 or 23:59 ?

    // Check if configuration button is pressed OR configuration is not initialized
    if (digitalRead(CONFIG_PIN) == LOW || !config.initialized || strlen(config.wifiSSID) == 0) {
        if (digitalRead(CONFIG_PIN) == LOW) {
            Serial.println("Configuration button pressed - entering setup mode");
        } else {
            Serial.println("No WiFi configuration found - entering setup mode");
        }

        startConfigMode();
        // startConfigMode() runs forever, so we never reach here
    }

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
        Serial.println("Time already synced less than an hour ago, no need to connect to WiFi.");
    }
    
    // Compute the image to display on the flipdot based on the current time
    uint16_t xMinutePosition = 0;
    char buf[4];
    uint16_t xBox = 0;
    uint16_t boxWidth = 0;

    // minutes in a char array
    sniprintf(buf, 4, "%02d", timeinfo.tm_min);
    
    // calculate x position of minutes, based on the hour and minute
    xMinutePosition = ((((timeinfo.tm_hour%12)/12.0)*(displayWidth)) + ((timeinfo.tm_min/60.0)*(14.0)));

    // if the hour is between 3 and 9, draw minutes upside down, otherwise draw normally
    if (timeinfo.tm_hour%12 >=3 && timeinfo.tm_hour%12 < 9) {
        dotFlippersMatrix.drawCharsUpSideDown(xMinutePosition, 0, buf, 0xFF);
    } else {
        dotFlippersMatrix.setCursor(xMinutePosition, 0);
        dotFlippersMatrix.print(String(buf));
    }

    // draw a box starting or ending at 0, up to the position of the minutes, to represent the hours
    if (timeinfo.tm_hour >= 12 && timeinfo.tm_hour < 24) { // box following the digits
        if (xMinutePosition < displayWidth-14) { // only if there is space to draw the box
            xBox = xMinutePosition+14;
            boxWidth = displayWidth - (xMinutePosition+14);
        }
    } else { // box preceding the digits
        if (xMinutePosition > 3 && xMinutePosition < displayWidth-17) { 
            xBox = 0;
            boxWidth = xMinutePosition-3;
        } else if (xMinutePosition >= displayWidth-17) {
            // add an offset to the start of the box when time is arount 11:55
            // to avoid the last minute digit to touch the start of the box (circular display).
            xBox = 3-(displayWidth-(xMinutePosition+11));
            boxWidth = displayWidth-14-xBox;
        }
    }
    dotFlippersMatrix.fillRect(xBox, 2, boxWidth, 3, 0xFF);

    // refresh flipdot display
    dotFlippersMatrix.display();

    Serial.println(&timeinfo, "%A %d %B %Y %H:%M:%S");
    Serial.println("Previous wifi sync hour: " + String(hourOfLastSinceSync));

    disableWiFi();

    int compensationDelayS = 0;
    // we can reduce the deep sleep time to wake up closer to the next minute
    if (timeinfo.tm_sec > 3) compensationDelayS = 2; 
    if (timeinfo.tm_sec > 20) compensationDelayS = 7; 

    goToDeepSleep1m(compensationDelayS);
}

void loop() {


}




