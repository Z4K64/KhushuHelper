#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

Preferences preferences;
WebServer server(80);

time_t lastTriggerTime = 0;  // Keeps track of last triggered time
//bool firstExecutionDone = false;

const char* apSSID = "ESP32_Setup";
const char* apPassword = "12345678";

// API URL for Quran translation
String apiUrl = "https://api.alquran.cloud/v1/edition/language/en"; //"https://api.alquran.cloud/v1/ayah/" + String(surah) + ":" + String(verse) + "/en.sahih";

// Base URL for image of verse
String imageUrl = "http://khushuhelper.infy.uk/verses/";

std::vector<String> fetchTranslators(String url);


bool shouldReboot = false;

// Discord constants
// Bot Token
const char* botToken = "DISCORD_BOT_TOKEN";

// Channel ID's

const char* general_ChannelId = "GENERAL_DISCORD_CHANNEL";
const char* surahVerseSelection_ChannelId = "SURAHVERSESELECTION_DISCORD_CHANNEL";
const char* confirmedSelection_ChannelId = "CONFIRMEDSELECTION_DISCORD_CHANNEL";
const char* sentVerses_ChannelId = "SENTVERSES_DISCORD_CHANNEL";


const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Setup</title>
<script>
  window.onload = function() {
    const timeEl = document.getElementById("time");
    const startStr = timeEl.dataset.start;
    const startTime = new Date(startStr.replace(" ", "T")); // Convert to JS Date

    if (isNaN(startTime.getTime())) {
      console.error("Invalid time format");
      return;
    }

    function updateTime() {
      const now = new Date(startTime.getTime() + (Date.now() - window.pageLoadTime));
      const formatted = now.toLocaleString('en-GB', {
        hour12: false,
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
      }).replace(",", "");
      timeEl.textContent = formatted;
    }

    window.pageLoadTime = Date.now();
    updateTime();
    setInterval(updateTime, 1000);
  };
</script>
</head>
<body>
  <h2>WiFi Setup</h2>
   <p><strong>Current Time:</strong> <span id="time" data-start="%LOCALTIME%">%LOCALTIME%</span></p>
  <form action="/save" method="post">
    SSID: 
    <select name="ssid">%OPTIONS%</select><br>
    Password: <input type="password" name="password" value="%PASSWORD%"><br>
    Frequency (hours): <input type="number" name="frequency" min="1" value="%FREQUENCY%"><br>
    Start Time (HH:MM): <input type="time" name="start" value="%START%"><br>
    Translator: 
    <select name="translator">%TRANSLATORS%</select><br>
    Time Zone: 
    <select name="timezone">%TIMEZONES%</select><br>
    <input type="submit" value="Save Settings">
</form>
<form action="/clear" method="post" style="margin-top:10px;">
  <input type="submit" value="Clear Settings">
</form>

  </form>
  <h3>Current Saved Settings:</h3>
  <ul>
    <li><strong>SSID:</strong> %SSID%</li>
    <li><strong>Password:</strong> %PASSWORD%</li>
    <li><strong>Frequency:</strong> %FREQUENCY% hours</li>
    <li><strong>Start Time:</strong> %START%</li>
    <li><strong>Translator:</strong> %TRANSLATOR%</li>
    <li><strong>Time Zone:</strong> %TIMEZONE%</li>
  </ul>

  <h2>Firmware Update</h2>
  <form method="POST" action="/update" enctype="multipart/form-data">
    <input type="file" name="update">
    <input type="submit" value="Update Firmware">
  </form>

  <form action="/reboot" method="post">
    <input type="submit" value="Reboot Now">
  </form>
</body>
</html>
)rawliteral";


std::vector<String> fetchTranslators(String url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return {"Failed to fetch translators."};
  }

  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("API Response: " + response);

    // Parse JSON response
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
      Serial.println("JSON parsing error");
      return {"Failed to parse translation."};
    }
    JsonArray data = doc["data"].as<JsonArray>();

    std::vector<String> names;
    for (JsonObject item : data) {
      if (item["name"].as<String>() != "Transliteration"){
        //Serial.println("True");
        names.push_back(item["name"].as<String>());
      }
    }
    //String translators = doc["data"]["name"].as<String>();
    http.end();
    return names;
  } else {
    Serial.println("Error in API request: " + String(httpResponseCode));
    http.end();
    return {"Failed to fetch translation."};//"Failed to fetch translation.";
  }
}


String getWiFiOptions() {
  String options = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }
  return options;
}

String getTimeZoneOptions(String selected) {
  struct TimeZoneOption {
    const char* label;
    const char* value;
  };

  TimeZoneOption timeZones[] = {
    {"UTC", "UTC0"},
    {"UK (BST)", "GMT0BST,M3.5.0/1,M10.5.0/2"},
    {"US Eastern", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"US Central", "CST6CDT,M3.2.0/2,M11.1.0/2"},
    {"US Mountain", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {"US Pacific", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {"India (IST)", "IST-5:30"},
    {"Australia (AEST)", "AEST-10AEDT,M10.1.0,M4.1.0"}
  };

  String html = "";
  for (const auto& z : timeZones) {
    html += "<option value='" + String(z.value) + "'";
    if (String(z.value) == selected) html += " selected";
    html += ">" + String(z.label) + "</option>";
  }
  return html;
}


void handleRoot() {
  preferences.begin("settings", true);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  int frequency = preferences.getInt("frequency", 1);
  String start = preferences.getString("start", "08:00");
  String savedTranslator = preferences.getString("translator", "");
  String savedTZ = preferences.getString("timezone", "UTC0");
  preferences.end();

// Set the timezone and get the current time
  setenv("TZ", savedTZ.c_str(), 1);
  tzset();
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  String currentTime = String(timeStr);

  String page = htmlForm;
  String options = "";
  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; ++i) {
    String ssidOption = WiFi.SSID(i);
    options += "<option value='" + ssidOption + "'";
    if (ssidOption == savedSSID) {
      options += " selected";
    }
    options += ">" + ssidOption + "</option>";
  }

  // Fetch the translation from the API
  std::vector<String> translators = fetchTranslators(apiUrl);
  // Build Translator options
  String translatorOptions = "";
  for (const auto& t : translators) {
    translatorOptions += "<option value='" + t + "'";
    if (t == savedTranslator) translatorOptions += " selected";
    translatorOptions += ">" + t + "</option>";
  }

  page.replace("%OPTIONS%", options);
  page.replace("%SSID%", savedSSID);
  page.replace("%PASSWORD%", savedPassword);
  page.replace("%FREQUENCY%", String(frequency));
  page.replace("%START%", start);
  page.replace("%TRANSLATORS%", translatorOptions);
  page.replace("%TRANSLATOR%", savedTranslator);
  page.replace("%TIMEZONES%", getTimeZoneOptions(savedTZ));
  page.replace("%TIMEZONE%", savedTZ);
  page.replace("%LOCALTIME%", currentTime);

  server.send(200, "text/html", page);
}


void handleSave() {
  preferences.begin("settings", false);
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("password", server.arg("password"));
  preferences.putInt("frequency", server.arg("frequency").toInt());
  preferences.putString("start", server.arg("start"));
  preferences.putString("translator", server.arg("translator"));
  preferences.putString("timezone", server.arg("timezone"));
  preferences.end();

  server.send(200, "text/html", "<h3>Settings Saved. Rebooting...</h3>");
  shouldReboot = true;
}

void handleReboot() {
  server.send(200, "text/html", "<h3>Rebooting...</h3>");
  shouldReboot = true;
}

void handleUpdate() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      Update.printError(Serial);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      server.send(200, "text/plain", "Firmware updated successfully. Rebooting...");
      shouldReboot = true;
    } else {
      Update.printError(Serial);
      server.send(500, "text/plain", "Firmware update failed.");
    }
  }
}

void handleClear() {
  preferences.begin("settings", false);
  preferences.clear();
  preferences.end();
  server.send(200, "text/html", "<h3>Settings Cleared. Rebooting...</h3>");
  shouldReboot = true;
}

void softAPEn() {
  IPAddress localIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(apSSID, apPassword);

  Serial.print("📡 SoftAP IP Address: ");
  Serial.println(WiFi.softAPIP());
}



void tryAutoConnectWiFi() {
  preferences.begin("settings", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  String savedTZ = preferences.getString("timezone", "");
  preferences.end();

  Serial.println("Attempting to connect with saved credentials:");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);

  if (ssid != "") {
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Connected to WiFi!");
      Serial.print("📡 IP Address: ");
      Serial.println(WiFi.localIP());

      // 🕒 Setup NTP time
      configTzTime(savedTZ.c_str(), "pool.ntp.org", "time.nist.gov");

      // Optional: wait up to 5 seconds for sync
      struct tm timeinfo;
      for (int i = 0; i < 10; i++) {
        if (getLocalTime(&timeinfo)) {
          Serial.println("✅ NTP time synced.");
          // Print current time (this will happen every time the block runs)
          char buf[64];
          strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
          Serial.println(buf);
          break;
        }
        delay(500);
      }

    } else {
      Serial.println("\n❌ Failed to connect. Starting Access Point mode...");
      softAPEn();
    }
  } else {
    Serial.println("⚠️ No saved WiFi credentials found. Starting Access Point mode...");
    softAPEn();
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  tryAutoConnectWiFi();
  

  Serial.println("Web server starting...");
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/update", HTTP_POST, []() {}, handleUpdate);
  server.on("/clear", HTTP_POST, handleClear);
  server.begin();
}

void loop() {
  server.handleClient();

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t now = mktime(&timeinfo); // Current time in seconds

    // Read saved preferences
    preferences.begin("settings", true);
    String startStr = preferences.getString("start", "08:00");
    int frequencyHours = preferences.getInt("frequency", 1);
    preferences.end();

    // Parse start time HH:MM
    int startHour = startStr.substring(0, 2).toInt();
    int startMin = startStr.substring(3, 5).toInt();

    // Create today's start time as a time_t
    struct tm startTime = timeinfo;
    startTime.tm_hour = startHour;
    startTime.tm_min = startMin;
    startTime.tm_sec = 0;
    time_t firstTrigger = mktime(&startTime);
    //Serial.println(String(ctime(&firstTrigger)));
    // Check if it's time to trigger
    if (now >= firstTrigger && (lastTriggerTime == 0 || difftime(now, lastTriggerTime) >= frequencyHours * 3600)) {
      Serial.println("⏰ Scheduled action triggered at: " + String(ctime(&now)));
      lastTriggerTime = now;

      // You can perform any action here, such as:
      // digitalWrite, send a message, etc.
    }
  }

  if (shouldReboot) {
    delay(1000);
    ESP.restart();
  }
}


// Function to send a message to Discord
bool sendMessageToDiscord(const String& message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return false;
  }

  HTTPClient http;
  String url = "https://discord.com/api/v9/channels/" + String(general_ChannelId) + "/messages";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bot " + String(botToken));

  // Format payload to send the message
  String payload = "{\"content\":\"" + message + "\"}";
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Discord Response: " + response);
  } else {
    Serial.println("Error in sending message: " + String(httpResponseCode));
  }

  http.end();
  return httpResponseCode == 200;
}
