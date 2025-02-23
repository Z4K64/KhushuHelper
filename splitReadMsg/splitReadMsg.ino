#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

//#define MAX_PARTS 10  // Define the maximum number of parts

// WiFi credentials
const char* ssid = "YOUR_WIFI"; 
const char* password = "WIFI_PASSWORD"; 

// Discord credentials
const char* botToken = "DISCORD_BOT_TOKEN";  
const char* readChannelId = "DISCORD_CHANNEL";   

//String messageParts[MAX_PARTS];  // Array to store message parts
//int partsCount = 0;  // Number of parts stored

// Structure to hold the split results
struct SplitResult {
  String parts[10];  // Fixed-size array to store up to 10 split parts
  int count;         // Number of parts found in the split operation
};

SplitResult splitMessage(String message, char delimiter);

int numSurah = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
//SPLITTING COMBOS
  // Fetch and process the last message
  String lastMessage = getLastMessageFromDiscord();
  if (lastMessage != "") {
    Serial.println("Last Message: " + lastMessage);

    // Check if the message contains "+"
    if (lastMessage.indexOf("+") != -1) {
      Serial.println("Message contains '+', splitting...");
      SplitResult combos = splitMessage(lastMessage, '+');
      
      // Example: Accessing parts later
      Serial.println("Accessing parts later:");
      for (int i = 0; i < combos.count; i++) {
        Serial.println("Stored Part [" + String(i) + "]: " + combos.parts[i]);
      }
      numSurah = combos.count;
      Serial.print("Number of Surahs: ");
      Serial.println(numSurah);
  //SPLITTING SURAH FROM VERSES
      SplitResult seperators[numSurah];
      
      for (int j = 0; j < numSurah; j++){
        if (combos.parts[j].indexOf(":") != -1) {
          seperators[j] = splitMessage(combos.parts[j], ':');
  
  
          Serial.println("Accessing parts later:");
          for (int k = 0; k < seperators[j].count; k++) {
            Serial.println("Stored Part [" + String(k) + "]: " + seperators[j].parts[k]);
          }
        }
        else{
          //SURAH WITH ALL VERSES ADDED
          Serial.println("all verses");
        }
      
      }
      
    } else {
      Serial.println("Message does not contain '+'.");
      numSurah = 1;
    }
    Serial.print("Number of Surahs: ");
    Serial.println(numSurah);
    
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


// Function to split a given string based on a specified delimiter
SplitResult splitMessage(String message, char delimiter) {
  SplitResult result;  // Create a struct to store results
  result.count = 0;    // Initialize the count of parts

  int startIndex = 0;  // Index where the next substring starts
  int endIndex = message.indexOf(delimiter);  // Find first occurrence of delimiter

  // Loop to find and extract substrings
  while (endIndex != -1 && result.count < 10) {  // Continue while delimiter exists and we have space in the array
    result.parts[result.count++] = message.substring(startIndex, endIndex);  // Store substring in array
    startIndex = endIndex + 1;  // Move start index past the delimiter
    endIndex = message.indexOf(delimiter, startIndex);  // Find next occurrence of delimiter
  }

  // Store the last part of the message (after the last delimiter)
  if (result.count < 10) {
    result.parts[result.count++] = message.substring(startIndex);
  }

  return result;  // Return the struct containing the split parts
}
