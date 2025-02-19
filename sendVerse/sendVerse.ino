#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI";
const char* password = "WIFI_PASSWORD";

// Discord credentials
const char* botToken = "DISCORD_BOT_TOKEN";
const char* channelId = "DISCORD_CHANNEL";

// Surah and Verse numbers (edit as needed)
int surah = 2;
int verse = 255;

// API URL for Quran translation
String apiUrl = "https://api.alquran.cloud/v1/ayah/" + String(surah) + ":" + String(verse) + "/en.sahih";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Construct the verse image link
  String imageUrl = "http://khushuhelper.infy.uk/verses/" + String(surah) + "_" + String(verse) + ".png";

  // Fetch the translation from the API
  String translation = fetchTranslation(apiUrl);

  // Send image link and translation to Discord
  if (sendMessageToDiscord(imageUrl) && sendMessageToDiscord(translation)) {
    Serial.println("Message sent successfully!");
  } else {
    Serial.println("Failed to send the message.");
  }
}

void loop() {
  // Loop is empty as we only send messages once on startup
}

// Function to fetch the translation from the API
String fetchTranslation(String url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return "Failed to fetch translation.";
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
      return "Failed to parse translation.";
    }

    String translation = doc["data"]["text"].as<String>();
    http.end();
    return translation;
  } else {
    Serial.println("Error in API request: " + String(httpResponseCode));
    http.end();
    return "Failed to fetch translation.";
  }
}

// Function to send a message to Discord
bool sendMessageToDiscord(const String& message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return false;
  }

  HTTPClient http;
  String url = "https://discord.com/api/v9/channels/" + String(channelId) + "/messages";

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
