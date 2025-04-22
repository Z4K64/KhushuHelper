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

// Lookup table: key 1 => 100, key 2 => 200, ..., key 5 => 500
const int valueMap[] = {
  7, 286, 200, 176, 120, 165, 206, 75, 129, 109,
  123, 111, 43, 52, 99, 128, 111, 110, 98, 135,
  112, 78, 118, 64, 77, 227, 93, 88, 69, 60,
  34, 30, 73, 54, 45, 83, 182, 88, 75, 85,
  54, 53, 89, 59, 37, 35, 38, 29, 18, 45,
  60, 49, 62, 55, 78, 96, 29, 22, 24, 13,
  14, 11, 11, 18, 12, 12, 30, 52, 52, 44,
  28, 28, 20, 56, 40, 31, 50, 40, 46, 42,
  29, 19, 36, 25, 22, 17, 19, 26, 30, 20,
  15, 21, 11, 8, 8, 19, 5, 8, 8, 11,
  11, 8, 3, 9, 5, 4, 7, 3, 6, 3,
  5, 4, 5, 6
};

const int NUM_KEYS = sizeof(valueMap) / sizeof(valueMap[0]);

// Function to safely look up value based on 1-based key
int lookup(int key) {
  if (key >= 1 && key <= NUM_KEYS) {
    return valueMap[key - 1];
  }
  return -1;  // Key not found
}

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
      Serial.println("Slitting combos:");
      for (int i = 0; i < combos.count; i++) {
        Serial.println("Stored Part [" + String(i) + "]: " + combos.parts[i]);
      }
      numSurah = combos.count;
      int chosenSurahs[numSurah];
      int chosenVerses[numSurah][286] = {0};
      int verseIndex = 0;
      Serial.print("Number of Surahs: ");
      Serial.println(numSurah);
  //SPLITTING SURAH FROM VERSES
      SplitResult seperators[numSurah];
      
      for (int j = 0; j < numSurah; j++){
        verseIndex = 0;
        if (combos.parts[j].indexOf(":") != -1) {
          seperators[j] = splitMessage(combos.parts[j], ':');
  
  
          Serial.println("Splitting Surah from Verse:");
          for (int k = 0; k < seperators[j].count; k++) {
            Serial.println("Stored Part [" + String(k) + "]: " + seperators[j].parts[k]);
          }

          chosenSurahs[j] = seperators[j].parts[0].toInt();  //Need to add sting to int
      // SPLITTING VERSE SELECTION
          SplitResult msgVerses[numSurah];

          if (seperators[j].parts[1].indexOf(",") != -1){
            msgVerses[j] = splitMessage(seperators[j].parts[1], ',');
            Serial.println("Splitting selected verses:");
            for (int l = 0; l < msgVerses[j].count; l++) {
              Serial.println("Stored Part [" + String(l) + "]: " + msgVerses[j].parts[l]);

              SplitResult selection;
              if (msgVerses[j].parts[l].indexOf("-") != -1){
                Serial.println("Splitting selection:");
                selection = splitMessage(msgVerses[j].parts[l], '-');

                for (int m = 0; m < selection.count; m++) {
                  Serial.println("Stored Part [" + String(m) + "]: " + selection.parts[m]);

                  selection.parts[m].replace(" ", ""); 
                }
                int startVerse = selection.parts[0].toInt();
                int endVerse = selection.parts[1].toInt();
                int rangeVerse = endVerse - startVerse;
                //Serial.println(startVerse);
                //Serial.println(endVerse);
                Serial.println("range");
                //Serial.println(selection.parts[0]);
                //selection.parts[0].replace(" ", "");
                //Serial.println(selection.parts[0]);

                for (int n = startVerse; n < (endVerse + 1); n++) {
                  //Serial.println(n);
                  chosenVerses[j][verseIndex] = n;
                  verseIndex += 1;
                }
              }
              else{
                Serial.println("Non Selection");
                msgVerses[j].parts[l].replace(" ", "");
                Serial.println(msgVerses[j].parts[l]);
                //Serial.println(chosenVerses[j][verseIndex]);
                chosenVerses[j][verseIndex] = msgVerses[j].parts[l].toInt();
                verseIndex += 1;                
              }
            }            
            
          }
          
        }
        else{
          //SURAH WITH ALL VERSES ADDED
          Serial.println("all verses");
          chosenSurahs[j] = combos.parts[j].toInt(); //Need to add sting to int
          int numOfVerse = lookup(chosenSurahs[j]);  
             
          for (int o = 0; o < numOfVerse; o++){
            chosenVerses[j][o] = o + 1;
          }  
        }
      
      }
      for (int j = 0; j < numSurah; j++){
        Serial.println("Chosen Surah: ");
        Serial.println(chosenSurahs[j]);
        Serial.println("Chosen Verses: ");
        for (int n = 0; n < 286; n++){
          if (chosenVerses[j][n] != 0){
            
            //Serial.print(n);
            
            Serial.print(chosenVerses[j][n]);
            Serial.print(" ");
          }
        }
        Serial.println();
        
      }
      
    } else {
      Serial.println("Message does not contain '+'.");
      numSurah = 1;
    }
    //Serial.print("Number of Surahs: ");
    //Serial.println(numSurah);
    
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
