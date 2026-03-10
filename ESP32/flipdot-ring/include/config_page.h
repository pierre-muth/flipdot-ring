#include <Arduino.h>

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