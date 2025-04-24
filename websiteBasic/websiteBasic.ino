#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>

Preferences preferences;
WebServer server(80);

const char* apSSID = "ESP32_Setup";
const char* apPassword = "12345678";

bool shouldReboot = false;

const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Setup</title></head>
<body>
  <h2>WiFi Setup</h2>
  <form action="/save" method="post">
    SSID: 
    <select name="ssid">%OPTIONS%</select><br>
    Password: <input type="password" name="password" value="%PASSWORD%"><br>
    Frequency (hours): <input type="number" name="frequency" min="1" value="%FREQUENCY%"><br>
    Start Time (HH:MM): <input type="time" name="start" value="%START%"><br>
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


String getWiFiOptions() {
  String options = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }
  return options;
}

void handleRoot() {
  preferences.begin("settings", true);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  int frequency = preferences.getInt("frequency", 1);
  String start = preferences.getString("start", "08:00");
  preferences.end();

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

  page.replace("%OPTIONS%", options);
  page.replace("%SSID%", savedSSID);
  page.replace("%PASSWORD%", savedPassword);
  page.replace("%FREQUENCY%", String(frequency));
  page.replace("%START%", start);

  server.send(200, "text/html", page);
}


void handleSave() {
  preferences.begin("settings", false);
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("password", server.arg("password"));
  preferences.putInt("frequency", server.arg("frequency").toInt());
  preferences.putString("start", server.arg("start"));
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
  if (shouldReboot) {
    delay(1000);
    ESP.restart();
  }
}
