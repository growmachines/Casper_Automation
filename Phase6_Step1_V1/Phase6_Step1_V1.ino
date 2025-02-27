#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <LittleFS.h> // Include LittleFS library
#include <ArduinoJson.h>
#include <ESP32Time.h>
#include <RTClib.h>



RTC_DS3231 rtcDS3231;

// Preferences for saving STA credentials and switch states
Preferences preferences;

// Wi-Fi AP Mode Credentials
const char* apSSID = "Casper_Automation";
const char* apPassword = "Casper123";
unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 86400000; // 24 hours

// Static IP Configuration
IPAddress staticIP(192, 168, 0, 60);      // Forced static IP for STA Mode
IPAddress gateway(192, 168, 0, 1);        // Default gateway for GrowMachines network
IPAddress subnet(255, 255, 255, 0);       // Subnet mask

// GPIO Pins for 20 switches
const int switchPins[20] = {0, 15, 5, 18, 19, 4, 21, 23, 22, 17, 
                            1, 16, 27, 14, 12, 13, 26, 25, 33, 32};

String getCurrentMode() {
    String mode;
    String ssid = "";

    if (WiFi.status() == WL_CONNECTED) { // Check STA mode connection first
        mode = "STA";
        ssid = WiFi.SSID();
    } else if (WiFi.getMode() & WIFI_AP) { // Check if AP mode is active
        mode = "AP";
    } else {
        mode = "UNKNOWN";
    }

    String response = "{\"mode\":\"" + mode + "\", \"ssid\":\"" + ssid + "\"}";
    return response;
}

// Web Server and WebSocket server
WebServer server(80);
WebSocketsServer webSocket(81);

// System States
bool isSoftAP = false;
String currentMode = "STA";

// --- Function to broadcast switch states via WebSocket ---
void broadcastSwitchState(int switchNumber, int state) {
  String message = "{\"switch\": " + String(switchNumber) + ", \"state\": \"" + (state ? "ON" : "OFF") + "\"}";
  webSocket.broadcastTXT(message);
}

// --- Save Switch State to Preferences ---
void saveSwitchState(int switchNumber, int state) {
  preferences.putInt(("Switch_" + String(switchNumber)).c_str(), state);
  broadcastSwitchState(switchNumber, state);
}

// --- Handler: Turn a switch ON ---
void handleSwitchOn() {
  int switchNumber = server.arg("id").toInt();
  if (switchNumber >= 1 && switchNumber <= 20) {
    digitalWrite(switchPins[switchNumber - 1], HIGH);
    saveSwitchState(switchNumber, HIGH);
    server.send(200, "text/plain", "Switch " + String(switchNumber) + " turned ON");
  } else {
    server.send(400, "text/plain", "Invalid switch number");
  }
}

// --- Handler: Turn a switch OFF ---
void handleSwitchOff() {
  int switchNumber = server.arg("id").toInt();
  if (switchNumber >= 1 && switchNumber <= 20) {
    digitalWrite(switchPins[switchNumber - 1], LOW);
    saveSwitchState(switchNumber, LOW);
    server.send(200, "text/plain", "Switch " + String(switchNumber) + " turned OFF");
  } else {
    server.send(400, "text/plain", "Invalid switch number");
  }
}

// --- Handler: Return Current IP Address ---
void handleCurrentIP() {
  String ipAddress = currentMode == "STA" ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  server.send(200, "text/plain", "Current IP Address: " + ipAddress);
}

// --- Handler: Return Switch States as JSON ---
void handleStatus() {
  String json = "{";
  for (int i = 0; i < 20; i++) {
    int state = preferences.getInt(("Switch_" + String(i + 1)).c_str(), LOW);
    json += "\"Switch_" + String(i + 1) + "\":\"" + (state ? "ON" : "OFF") + "\"";
    if (i < 19) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

// --- Factory Reset: Clear all Preferences ---
void handleFactoryReset() {
  Serial.println("Factory Reset Initiated...");
  preferences.clear();
  preferences.end();
  server.send(200, "text/plain", "Factory Reset Successful. Rebooting...");
  delay(1000);
  ESP.restart();
}

void handleSaveAPConfig() {
    if (!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Missing parameter: SSID");
        return;
    }

    String ssid = server.arg("ssid");
    String WiFiPassword = server.hasArg("WiFiPassword") ? server.arg("WiFiPassword") : ""; // Handle open networks
    String alternativeIP = server.hasArg("alternativeIP") ? server.arg("alternativeIP") : ""; // Optional alternative IP

    preferences.putString("sta_ssid", ssid);
    preferences.putString("sta_password", WiFiPassword);

    // Update alternative IP if provided
    IPAddress local_IP;
    if (!alternativeIP.isEmpty() && local_IP.fromString(alternativeIP)) {
        preferences.putString("alternative_ip", alternativeIP);
    } else if (!preferences.isKey("alternative_ip")) {
        preferences.putString("alternative_ip", "192.168.0.60"); // Default IP if no previous value exists
    }

    Serial.println("Received SSID: " + ssid);
    Serial.println("Received WiFiPassword: " + (WiFiPassword.isEmpty() ? "<Open Network>" : WiFiPassword));
    Serial.println("Alternative IP: " + preferences.getString("alternative_ip"));

    // Attempt to connect to Wi-Fi immediately
    WiFi.mode(WIFI_AP_STA); // Ensure both SoftAP and STA modes are active
    WiFi.softAP(apSSID, apPassword); // Start SoftAP mode

    Serial.println("SoftAP Mode active at IP: " + WiFi.softAPIP().toString());

    // Apply the alternative IP
    String savedIP = preferences.getString("alternative_ip", "192.168.0.60");
    if (local_IP.fromString(savedIP)) {
        IPAddress gateway(192, 168, 0, 1); // Adjust gateway as needed
        IPAddress subnet(255, 255, 255, 0);
        WiFi.config(local_IP, gateway, subnet);
        Serial.println("Configured static IP: " + local_IP.toString());
    }

    WiFi.begin(ssid.c_str(), WiFiPassword.c_str());

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
        delay(1000);
        Serial.println("Connecting to Wi-Fi...");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Wi-Fi. IP Address: " + WiFi.localIP().toString());
        server.send(200, "text/plain", "Settings saved and connected to Wi-Fi successfully.");
    } else {
        Serial.println("Failed to connect to Wi-Fi.");
        server.send(500, "text/plain", "Settings saved but failed to connect to Wi-Fi. Please check your credentials.");
    }
}




// --- Handler: Save STA Configuration ---
void handleSaveSTAConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }

  // Parse incoming JSON
  DynamicJsonDocument timerDoc(512);
  deserializeJson(timerDoc, server.arg("plain"));

  int switchNumber = timerDoc["switchNumber"];
  String switchName = timerDoc["switchName"];

  // Corrected line for saving switch name
  String key = "switch_name_" + String(switchNumber);
  preferences.putString(key.c_str(), switchName);

  // Save On/Off timers
  JsonArray timers = timerDoc["timers"];
  for (int i = 0; i < timers.size(); i++) {
    String onKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_on";
    String offKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_off";

    preferences.putString(onKey.c_str(), timers[i]["on"].as<String>());
    preferences.putString(offKey.c_str(), timers[i]["off"].as<String>());
  }

  // Save icon
  String icon = timerDoc["icon"];
  String iconKey = "icon_" + String(switchNumber);
  preferences.putString(iconKey.c_str(), icon);

  server.send(200, "application/json", "{\"status\": \"success\"}");
}

// --- Handler: Load On/Off Timers ---
void handleLoadOnOffTimers() {
  int switchNumber = preferences.getInt("selected_switch", 1); // Default switch number

  DynamicJsonDocument timerDoc(512);
  JsonArray timers = timerDoc.createNestedArray("timers");

  for (int i = 0; i < 5; i++) {
    String onKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_on";
    String offKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_off";

    String onTime = preferences.getString(onKey.c_str(), "00:00");
    String offTime = preferences.getString(offKey.c_str(), "00:00");

    JsonObject timer = timers.createNestedObject();
    timer["on"] = onTime;
    timer["off"] = offTime;
  }

  String jsonResponse;
  serializeJson(timerDoc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}
void listFilesInRoot() {
    Serial.println("Listing files in LittleFS:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.println(file.name());
        file = root.openNextFile();
    }
}


// List icon files in LittleFS
// Backend function to handle listing icons
void handleListIcons() {
    String json = "[";
    bool first = true;

    // Open the root directory to list files
    File root = LittleFS.open("/images"); // Open the /images directory
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open /images directory or it is not a directory");
        server.send(500, "application/json", "{\"error\":\"Failed to open images directory\"}");
        return;
    }

    // Iterate over files in the /images directory
    File file = root.openNextFile();
    while (file) {
        if (String(file.name()).endsWith(".jpg")) { // Only include .jpg files
            if (!first) json += ",";
            json += "\"" + String(file.name()) + "\""; // Add file name to JSON
            first = false;
        }
        file = root.openNextFile();
    }

    json += "]";

    // Send JSON response
    if (json == "[]") {
        Serial.println("No icons found in /images");
        server.send(404, "application/json", "{\"error\":\"No icons found\"}");
    } else {
        server.send(200, "application/json", json);
        Serial.println("Icons listed: " + json);
    }
}



// Save icon path
void handleSaveIconPath(int switchNumber, String iconPath) {
  String keyBuffer = "icon_" + String(switchNumber);
  char iconKey[50];
  keyBuffer.toCharArray(iconKey, sizeof(iconKey));
  preferences.putString(iconKey, iconPath);

  server.send(200, "text/plain", "Settings saved successfully");
}
void handleSaveIcon() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(128);
        deserializeJson(doc, server.arg("plain"));
        int switchNumber = doc["switch"];
        String icon = doc["icon"];

        String key = "icon_" + String(switchNumber);
        preferences.putString(key.c_str(), icon);

        server.send(200, "text/plain", "Icon saved");
    } else {
        server.send(400, "text/plain", "Invalid request");
    }
}



// Handle icon upload
void handleIconUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/images/" + upload.filename;
    File file = LittleFS.open(filename, "w");
    if (file) file.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = LittleFS.open("/images/" + upload.filename, "a");
    if (file) file.write(upload.buf, upload.currentSize);
    file.close();
  } else if (upload.status == UPLOAD_FILE_END) {
    server.send(200, "text/plain", "File uploaded successfully");
  }
}

// --- Handler: Wi-Fi Scan ---
void handleWiFiScan() {
  Serial.println("Initiating Wi-Fi Scan...");

  int networkCount = WiFi.scanNetworks(false, true); // Perform Wi-Fi scan
  if (networkCount == -1) {
    server.send(500, "text/plain", "Wi-Fi scan failed");
    Serial.println("Wi-Fi scan failed!");
    return;
  }

  // Handle no networks found
  if (networkCount == 0) {
    server.send(200, "application/json", "[]"); // Send an empty JSON array
    Serial.println("No networks found.");
    return;
  }

  // Construct JSON response with network details
  String json = "[";
  for (int i = 0; i < networkCount; i++) {
    if (i > 0) json += ","; // Add a comma before every subsequent object
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";               // Network SSID
    json += "\"signal\":" + String(WiFi.RSSI(i)) + ",";         // Signal strength (RSSI)
    json += "\"encryption\":" + String(WiFi.encryptionType(i)); // Encryption type
    json += "}";
  }
  json += "]";

  // Send the JSON response to the client
  server.send(200, "application/json", json);
  Serial.println("Wi-Fi Scan Complete. Networks Found: " + String(networkCount));
  Serial.println("Response Sent: " + json);
}

// --- Handler: Load STA Settings ---
void handleLoadSTASettings() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "Switch ID is missing");
    return;
  }
  int switchId = server.arg("id").toInt();
  String settingsKey = "Switch_" + String(switchId);

  String settings = preferences.getString(settingsKey.c_str(), "{}");
  server.send(200, "application/json", settings);
}

// --- Handler: Save STA Settings ---
void handleSaveSTASettings() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "Switch ID is missing");
    return;
  }
  int switchId = server.arg("id").toInt();
  String settingsKey = "Switch_" + String(switchId);
  String body = server.arg("plain"); // Raw JSON data sent by the client

  preferences.putString(settingsKey.c_str(), body);
  server.send(200, "text/plain", "Settings saved successfully");
}
void handleLoadSTAConfig() {
  int switchNumber = preferences.getInt("selected_switch", 1); // Default to switch 1
  String switchNameKey = "switch_name_" + String(switchNumber);
  String switchName = preferences.getString(switchNameKey.c_str(), ""); // Convert String to const char*

  // Initialize a JSON object
  DynamicJsonDocument timerDoc(512);
  JsonArray timers = timerDoc.createNestedArray("timers");

  // Load timers from preferences
  for (int i = 0; i < 5; i++) {
    JsonObject timer = timers.createNestedObject();
    String onKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_on";
    String offKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_off";
    timer["on"] = preferences.getString(onKey.c_str(), "00:00"); // Convert to const char*
    timer["off"] = preferences.getString(offKey.c_str(), "00:00"); // Convert to const char*
  }

  timerDoc["switchNumber"] = switchNumber;
  timerDoc["switchName"] = switchName;

  String jsonResponse;
  serializeJson(timerDoc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}


void listFiles() {
  Serial.println("Files in LittleFS:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

// Save AP Settings
void handleSaveApSettings() {
    if (!server.hasArg("ssid") || !server.hasArg("password") || !server.hasArg("utcOffset") || !server.hasArg("alternativeIP")) {
        server.send(400, "text/plain", "Missing AP settings.");
        return;
    }

    // Parse data
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String utcOffset = server.arg("utcOffset");
    String alternativeIP = server.arg("alternativeIP");

    // Save to Preferences
    preferences.putString("ap_ssid", ssid);
    preferences.putString("ap_password", password);
    preferences.putString("ap_utc_offset", utcOffset);
    preferences.putString("ap_alternative_ip", alternativeIP);

    // Send response
    server.send(200, "text/plain", "AP settings saved successfully.");

    // Debugging logs
    Serial.println("AP Settings saved:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("UTC Offset: " + utcOffset);
    Serial.println("Alternative IP: " + alternativeIP);
}


// Save STA Settings
void handleSaveStaSettings() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "No data received");
        return;
    }

    // Parse incoming JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    // Parse data
    int switchNumber = doc["switchNumber"];
    String switchName = doc["switchName"];
    String iconName = doc["iconName"];
    String username = doc["username"];
    String password = doc["password"];
    JsonArray timers = doc["timers"];

    // Save switch name, icon, username, and password
    preferences.putString(("switch_name_" + String(switchNumber)).c_str(), switchName);
    preferences.putString(("icon_" + String(switchNumber)).c_str(), iconName);
    preferences.putString("username", username);
    preferences.putString("password", password);

    // Save timers
    for (size_t i = 0; i < timers.size(); i++) {
        String onKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_on";
        String offKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_off";
        preferences.putString(onKey.c_str(), timers[i]["on"].as<String>());
        preferences.putString(offKey.c_str(), timers[i]["off"].as<String>());
    }

    // Send response
    server.send(200, "application/json", "{\"status\": \"success\"}");

    // Debugging logs
    Serial.printf("Saved settings for Switch %d:\n", switchNumber);
    Serial.printf("Switch Name: %s\nIcon: %s\nUsername: %s\nPassword: %s\n", switchName.c_str(), iconName.c_str(), username.c_str(), password.c_str());
    for (size_t i = 0; i < timers.size(); i++) {
        Serial.printf("Timer %d ON: %s, OFF: %s\n", i + 1, timers[i]["on"].as<String>().c_str(), timers[i]["off"].as<String>().c_str());
    }
}


void handleLoadSwitchSettings() {
    if (!server.hasArg("s")) {
        server.send(400, "application/json", "{\"error\":\"Switch number not provided\"}");
        return;
    }

    int switchNumber = server.arg("s").toInt();
    DynamicJsonDocument doc(512);

    // Retrieve switch name, icon, username, and password
    String switchName = preferences.getString(("switch_name_" + String(switchNumber)).c_str(), "Default Name");
    String iconName = preferences.getString(("icon_" + String(switchNumber)).c_str(), "/images/Button_Default_Image.jpg");
    String username = preferences.getString("username", "DefaultUser");
    String password = preferences.getString("password", "");
    Serial.printf("Retrieved Username: %s\n", username.c_str());
    Serial.printf("Retrieved Password: %s\n", password.c_str());

    // Retrieve timers
    JsonArray timers = doc.createNestedArray("timers");
    for (int i = 0; i < 5; i++) {
        String onKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_on";
        String offKey = "timer_" + String(switchNumber) + "_" + String(i + 1) + "_off";
        JsonObject timer = timers.createNestedObject();
        timer["on"] = preferences.getString(onKey.c_str(), "00:00");
        timer["off"] = preferences.getString(offKey.c_str(), "00:00");
    }

    // Add data to JSON response
    doc["switchName"] = switchName;
    doc["iconName"] = iconName;
    doc["username"] = username;
    doc["password"] = password;

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);

    // Debugging logs
    Serial.printf("Loaded settings for Switch %d:\n", switchNumber);
    Serial.printf("Switch Name: %s\nIcon: %s\nUsername: %s\nPassword: %s\n", switchName.c_str(), iconName.c_str(), username.c_str(), password.c_str());
    for (int i = 0; i < timers.size(); i++) {
        Serial.printf("Timer %d ON: %s, OFF: %s\n", i + 1, timers[i]["on"].as<String>().c_str(), timers[i]["off"].as<String>().c_str());
    }
}


void handleSetSystemTime() {
    if (!server.hasArg("time") || !server.hasArg("utcOffset")) {
        server.send(400, "text/plain", "Time or UTC Offset is missing.");
        return;
    }

    String time = server.arg("time");
    String utcOffset = server.arg("utcOffset");

    int hour = time.substring(0, 2).toInt();
    int minute = time.substring(3, 5).toInt();
    int second = time.substring(6, 8).toInt();
  if (!rtcDS3231.begin()) {
    Serial.println("RTC not available. Skipping time update.");
    return;
    }
    else
    {
    rtcDS3231.adjust(DateTime(2023, 1, 1, hour, minute, second)); // Update RTC
    preferences.putString("utcOffset", utcOffset);               // Save UTC offset
    preferences.putString("lastTime", rtcDS3231.now().timestamp());

    Serial.println("Time updated via SetSystemTime endpoint:");
    Serial.println("Time: " + time + " UTC Offset: " + utcOffset);

    server.send(200, "text/plain", "Time updated successfully.");
    }
}



void fetchTimeFromNTP() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (!rtcDS3231.begin()) {
    Serial.println("RTC not available. Skipping time update.");
    return;
    }
     if (getLocalTime(&timeinfo)) {
        rtcDS3231.adjust(DateTime(
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec
        ));
        Serial.println("Time synchronized from NTP.");
        preferences.putString("lastTime", rtcDS3231.now().timestamp());
    } else {
        Serial.println("Failed to sync time from NTP.");
    }
}

void handleGetSystemTime() {
    String time = preferences.getString("lastTime", "1970-01-01T00:00:00");
    String utcOffset = preferences.getString("utcOffset", "+02:00");
    String json = "{\"time\": \"" + time + "\", \"utcOffset\": \"" + utcOffset + "\"}";
    server.send(200, "application/json", json);
}


void setupRTC() {
    if (!rtcDS3231.begin()) {
        Serial.println("RTC not found. Skipping RTC initialization.");
        return; // Skip RTC initialization entirely
    }

    if (rtcDS3231.lostPower()) {
        Serial.println("RTC lost power. Setting default time.");
        rtcDS3231.adjust(DateTime(2023, 1, 1, 0, 0, 0)); // Set a default time
    }
}



void syncTimeWithNTP() {
    configTime(7200, 0, "pool.ntp.org", "time.nist.gov"); // Adjust for UTC+2
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) { // 5-second timeout
        String timeString = String(timeinfo.tm_year + 1900) + "-" +
                            String((timeinfo.tm_mon + 1) < 10 ? "0" : "") + String(timeinfo.tm_mon + 1) + "-" +
                            String(timeinfo.tm_mday < 10 ? "0" : "") + String(timeinfo.tm_mday) + "T" +
                            String(timeinfo.tm_hour < 10 ? "0" : "") + String(timeinfo.tm_hour) + ":" +
                            String(timeinfo.tm_min < 10 ? "0" : "") + String(timeinfo.tm_min) + ":" +
                            String(timeinfo.tm_sec < 10 ? "0" : "") + String(timeinfo.tm_sec);

        preferences.putString("lastTime", timeString);
        preferences.putString("utcOffset", "+02:00");
        Serial.println("Time synchronized from NTP: " + timeString);
    } else {
        Serial.println("Failed to synchronize with NTP.");
    }
}

void handleSetBrowserTime() {
    if (!server.hasArg("time") || !server.hasArg("utcOffset")) {
        server.send(400, "text/plain", "Missing time or UTC offset.");
        return;
    }

    String time = server.arg("time");
    String utcOffset = server.arg("utcOffset");

    preferences.putString("lastTime", time);
    preferences.putString("utcOffset", utcOffset);

    Serial.println("Time updated via browser:");
    Serial.println("Time: " + time);
    Serial.println("UTC Offset: " + utcOffset);

    server.send(200, "text/plain", "Time updated successfully.");
}



String getRTCTime() {
    if (!rtcDS3231.begin()) {
        return "RTC not initialized.";
    }
    DateTime now = rtcDS3231.now();
    return now.timestamp();
}
// Handler to log system time to Serial Monitor
void handleLogSystemTime() {
    if (!server.hasArg("time") || !server.hasArg("utcOffset")) {
        server.send(400, "text/plain", "Missing time or UTC offset.");
        return;
    }

    String time = server.arg("time");
    String utcOffset = server.arg("utcOffset");

    // Log time and offset to Serial Monitor
    Serial.println("System Time Received from Web Page:");
    Serial.println("Time: " + time);
    Serial.println("UTC Offset: " + utcOffset);

    // Respond to the web request
    server.send(200, "text/plain", "System time logged to Serial Monitor.");
}
void handleCurrentMode() {
    String modeInfo = getCurrentMode();
    server.send(200, "application/json", modeInfo);
}

// --- Setup HTTP Routes ---
void setupRoutes() {
    // Access Point (AP) Configuration Routes
    server.on("/SaveAPConfig", HTTP_POST, handleSaveAPConfig);       // Save Wi-Fi SSID and Password for AP mode
    server.on("/SaveApSettings", HTTP_POST, handleSaveApSettings);   // Save additional AP-specific settings

    // Station (STA) Configuration Routes
    server.on("/SaveSTAConfig", HTTP_POST, handleSaveSTAConfig);     // Save Wi-Fi SSID and Password for STA mode
    server.on("/SaveStaSettings", HTTP_POST, handleSaveStaSettings); // Save switch/timer settings for STA mode
    server.on("/LoadSTASettings", HTTP_GET, handleLoadSTASettings);  // Load STA-specific settings
    server.on("/LoadSTAConfig", HTTP_GET, handleLoadSTAConfig);      // Load STA Wi-Fi credentials
    server.on("/load-switch-settings", HTTP_GET, handleLoadSwitchSettings); // Load switch settings for STA
    server.on("/GetSystemTime", HTTP_GET, handleGetSystemTime);


    // System Time Routes
    server.on("/SetSystemTime", HTTP_POST, handleSetSystemTime);     // Set manual system time
    server.on("/GetSystemTime", HTTP_GET, handleGetSystemTime);      // Fetch current system time
    server.on("/LogSystemTime", HTTP_GET, handleLogSystemTime);
    server.on("/SetBrowserTime", HTTP_POST, handleSetBrowserTime);


    // File Handling Routes
    server.onNotFound(handleFileRequest); // Serve files dynamically from LittleFS

    // Switch Control Routes
    server.on("/SwitchOn", HTTP_GET, handleSwitchOn);   // Turn a switch ON
    server.on("/SwitchOff", HTTP_GET, handleSwitchOff); // Turn a switch OFF

    // System Status and Utilities
    server.on("/CurrentMode", HTTP_GET, handleCurrentMode);         // Get current mode (AP or STA)
    server.on("/CurrentIP", HTTP_GET, handleCurrentIP);             // Get current IP address
    server.on("/Status", HTTP_GET, handleStatus);                   // Get system status
    server.on("/WiFiScan", HTTP_GET, handleWiFiScan);               // Scan available Wi-Fi networks
    server.on("/FactoryReset", HTTP_POST, handleFactoryReset);     // Reset to factory settings

    // Icon Management Routes
    server.on("/list-icons", HTTP_GET, handleListIcons);    // List all available icons
    server.on("/save-icon", HTTP_POST, handleSaveIcon);     // Save a new icon

   
    
}


// --- Attempt STA Connection with Static IP ---
bool connectToSTA() {
  String ssid = preferences.getString("sta_ssid", "");
  String password = preferences.getString("sta_password", "");

  if (ssid.isEmpty()) {
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid.c_str(), password.c_str());
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 5) {
    delay(2000);
    retries++;
  }
  return WiFi.status() == WL_CONNECTED;
}

// --- Start Soft-AP Mode ---
void startSoftAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    Serial.println("AP mode started. IP Address: " + WiFi.softAPIP().toString());
}

void handleFileRequest() {
  String path = server.uri();
  Serial.println("Requested Path: " + path);

  if (path.endsWith("/")) path += "index.html"; // Serve index.html for root
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    Serial.println("Serving File: " + path);
    server.streamFile(file, getContentType(path));
    file.close();
  } else {
    Serial.println("File Not Found: " + path);
    server.send(404, "text/plain", "File Not Found");
  }
}



// Determine MIME type for served files
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".gif")) return "image/gif";
  if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}
void setupWiFi() {
    // Initialize SoftAP mode
    WiFi.mode(WIFI_AP_STA); // Enable both AP and STA modes
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID, apPassword); // Start SoftAP
    Serial.println("SoftAP Mode active at IP: " + WiFi.softAPIP().toString());

    // Load STA credentials from preferences
    String ssid = preferences.getString("sta_ssid", "");
    String password = preferences.getString("sta_password", "");
    String alternativeIP = preferences.getString("alternative_ip", "192.168.0.60"); // Default IP if not set

    // If no STA credentials, remain in SoftAP mode
    if (ssid.isEmpty()) {
        Serial.println("No STA credentials found. Staying in SoftAP mode.");
        return;
    }

    // Configure alternative or default static IP for STA mode
    IPAddress local_IP;
    if (local_IP.fromString(alternativeIP)) {
        WiFi.config(local_IP, IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));
        Serial.println("Configured alternative IP: " + local_IP.toString());
    } else {
        WiFi.config(IPAddress(192, 168, 0, 60), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));
        Serial.println("Using default static IP: 192.168.0.60");
    }

    // Attempt to connect to Wi-Fi in STA mode
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Attempting STA connection to SSID: " + ssid);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
        delay(1000);
        Serial.println("Retrying STA connection...");
        retries++;
    }

    // STA connection status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("STA Mode: Connected to Wi-Fi. IP Address: " + WiFi.localIP().toString());
    currentMode = "STA";
    } else {
        Serial.println("STA Mode: Connection failed. Retaining SoftAP mode for configuration.");
    currentMode = "AP";
    }


}





void checkWiFiStatus() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("STA Mode: Disconnected. SoftAP remains active.");
    } else {
        Serial.println("STA Mode: Connected to Wi-Fi.");
    }
}

// --- Setup ---
void setup() {
    Serial.begin(115200);
    Serial.println("System Initialization...");

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed! Formatting...");
        LittleFS.format(); // Optional: Format LittleFS
        listFiles();
        if (!LittleFS.begin()) {
            Serial.println("LittleFS Mount Failed Again!");
            return; // Halt if LittleFS fails to mount
        }
    }
    Serial.println("LittleFS Mounted Successfully!");

    // Initialize preferences
    preferences.begin("wifiCreds", false);
    Serial.println("Loading preferences...");
    Serial.printf("Last Time: %s\n", preferences.getString("lastTime", "Not Set").c_str());
    Serial.printf("UTC Offset: %s\n", preferences.getString("utcOffset", "+00:00").c_str());

    // Initialize RTC
    setupRTC();

    // Setup Wi-Fi and time synchronization
    setupWiFi();

    // Check time sources
    if (WiFi.status() == WL_CONNECTED) {
        syncTimeWithNTP(); // Try to fetch time from NTP
    } else if (rtcDS3231.begin()) {
        Serial.println("Using time from RTC.");
    } else {
        // Fallback to saved time
        String lastTime = preferences.getString("lastTime", "1970-01-01T00:00:00");
        String utcOffset = preferences.getString("utcOffset", "+00:00");
        Serial.println("Fallback: Using last saved time from preferences.");
        Serial.println("Time: " + lastTime + ", UTC Offset: " + utcOffset);
    }

    // Initialize GPIO for switches
    for (int i = 0; i < 20; i++) {
        pinMode(switchPins[i], OUTPUT);
        int savedState = preferences.getInt(("Switch_" + String(i + 1)).c_str(), LOW);
        digitalWrite(switchPins[i], savedState);
    }

    // Setup HTTP routes and WebSocket server
    setupRoutes();
    server.begin();
    webSocket.begin();

    Serial.println("Server started.");
    listFilesInRoot();
}



// --- Loop ---
void loop() {
  server.handleClient();
  webSocket.loop();

 // Check Wi-Fi status periodically
    checkWiFiStatus();

//periodically synchronize with NTP (STA mode) or fallback to RTC/AP mode:
    unsigned long now = millis();
    if (WiFi.status() == WL_CONNECTED && now - lastSyncTime > syncInterval) {
        syncTimeWithNTP();
        lastSyncTime = now;
    }
}
