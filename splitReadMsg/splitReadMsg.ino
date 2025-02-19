#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define MAX_PARTS 10  // Define the maximum number of parts

// WiFi credentials
const char* ssid = "YOUR_WIFI"; 
const char* password = "WIFI_PASSWORD"; 

// Discord credentials
const char* botToken = "DISCORD_BOT_TOKEN";  
const char* readChannelId = "DISCORD_CHANNEL";   

String messageParts[MAX_PARTS];  // Array to store message parts
int partsCount = 0;  // Number of parts stored

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Fetch and process the last message
  String lastMessage = getLastMessageFromDiscord();
  if (lastMessage != "") {
    Serial.println("Last Message: " + lastMessage);

    // Check if the message contains "+"
    if (lastMessage.indexOf("+") != -1) {
      Serial.println("Message contains '+', splitting...");
      splitMessage(lastMessage);
      
      // Example: Accessing parts later
      Serial.println("Accessing parts later:");
      for (int i = 0; i < partsCount; i++) {
        Serial.println("Stored Part [" + String(i) + "]: " + messageParts[i]);
      }
    } else {
      Serial.println("Message does not contain '+'.");
    }
  } else {
    Serial.println("Failed to fetch the last message.");
  }
}

void loop() {
  // Empty loop or add additional functionality if needed.
}

String getLastMessageFromDiscord() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return "";
  }

  HTTPClient http;
  String url = "https://discord.com/api/v9/channels/" + String(readChannelId) + "/messages?limit=1";
  
  http.begin(url);
  http.addHeader("Authorization", "Bot " + String(botToken));

  int httpResponseCode = http.GET();
  String messageContent = "";

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response: " + response);

    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      messageContent = doc[0]["content"].as<String>(); // Extract the message content
    } else {
      Serial.println("Failed to parse JSON!");
    }
  } else {
    Serial.println("Error fetching messages: " + String(httpResponseCode));
  }

  http.end();
  return messageContent;
}

// Function to split the message and store it in an array
void splitMessage(String message) {
  int startIndex = 0;
  int endIndex = message.indexOf("+");
  partsCount = 0;  // Reset count

  while (endIndex != -1 && partsCount < MAX_PARTS) {
    messageParts[partsCount++] = message.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    endIndex = message.indexOf("+", startIndex);
  }

  // Store the last part
  if (partsCount < MAX_PARTS) {
    messageParts[partsCount++] = message.substring(startIndex);
  }
}
