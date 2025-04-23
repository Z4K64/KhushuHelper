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

bool chosenSurahs[114] = {false};         // true if Surah is selected
bool chosenVerses[114][286] = {false};    // true if that specific verse is selected


void setup() {
  Serial.begin(115200);
  delay(1000);  // Give Serial time to start
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
        combos.parts[i].trim();
        Serial.println("Stored Part [" + String(i) + "]: " + combos.parts[i]);
      }
      numSurah = combos.count;

      int verseIndex = 0;
      Serial.print("Number of Surahs: ");
      Serial.println(numSurah);
  //SPLITTING SURAH FROM VERSES
      SplitResult seperators[numSurah];
      
      for (int j = 0; j < numSurah; j++){
        verseIndex = 0;
        combos.parts[j].trim();
        if (combos.parts[j].indexOf(":") != -1) {
          seperators[j] = splitMessage(combos.parts[j], ':');
          Serial.println("Splitting Surah from Verse:");
          for (int k = 0; k < seperators[j].count; k++) {
            seperators[j].parts[k].trim();
            Serial.println("Stored Part [" + String(k) + "]: " + seperators[j].parts[k]);
          }
          int surahValue = seperators[j].parts[0].toInt() -1;

          chosenSurahs[surahValue] = true;  //Need to add sting to int
      // SPLITTING VERSE SELECTION
          SplitResult msgVerses[numSurah];

          if (seperators[j].parts[1].indexOf(",") != -1){
            msgVerses[j] = splitMessage(seperators[j].parts[1], ',');
            Serial.println("Splitting selected verses:");
            for (int l = 0; l < msgVerses[j].count; l++) {
              msgVerses[j].parts[l].trim();
              Serial.println("Stored Part [" + String(l) + "]: " + msgVerses[j].parts[l]);
              //SPLITTING RANGE Selection
              SplitResult selection;
              if (msgVerses[j].parts[l].indexOf("-") != -1){
                Serial.println("Splitting selection:");
                selection = splitMessage(msgVerses[j].parts[l], '-');

                for (int m = 0; m < selection.count; m++) {
                  Serial.println("Stored Part [" + String(m) + "]: " + selection.parts[m]);                 
                }
                selection.parts[0].trim();
                selection.parts[0].trim();
                int startVerse = selection.parts[0].toInt() -1;
                int endVerse = selection.parts[1].toInt() -1;
                int rangeVerse = endVerse - startVerse;
                Serial.println("range");

                for (int n = startVerse; n <= endVerse; n++) {
                  chosenVerses[surahValue][n] = true;
                  verseIndex += 1;
                }
              }
              else{
                Serial.println("Non Selection");
                Serial.println(msgVerses[j].parts[l]);
                msgVerses[j].parts[l].trim();
                int msgValue = msgVerses[j].parts[l].toInt() -1;
                chosenVerses[surahValue][msgValue] = true;
                verseIndex += 1;                
              }
            }

          }
          else{

            //SPLITTING RANGE Selection
            SplitResult selection;
            seperators[j].parts[1].trim();

            if (seperators[j].parts[1].indexOf("-") != -1){
              Serial.println("Splitting selection:");
              selection = splitMessage(seperators[j].parts[1], '-');

              for (int m = 0; m < selection.count; m++) {
                Serial.println("Stored Part [" + String(m) + "]: " + selection.parts[m]);                  
              }
              selection.parts[0].trim();
              selection.parts[1].trim();
              int startVerse = selection.parts[0].toInt() -1;
              int endVerse = selection.parts[1].toInt() -1;
              int rangeVerse = endVerse - startVerse;
              
              Serial.println("range");

              for (int n = startVerse; n <= endVerse; n++) {
                chosenVerses[surahValue][n] = true;
                verseIndex += 1;
              }
            }
            else{
              Serial.println("Non Selection");
              
              Serial.println(seperators[j].parts[1]);
              
              seperators[j].parts[1].trim();
              int msgValue = seperators[j].parts[1].toInt() -1;
              chosenVerses[surahValue][msgValue] = true;
              verseIndex += 1;                
            }
          }  
        }
        else{
          //SURAH WITH ALL VERSES ADDED
          Serial.println("all verses");
          combos.parts[j].trim();
          int surahValue = combos.parts[j].toInt() -1;
          Serial.println(surahValue +1);
          chosenSurahs[surahValue] = true; //Need to add sting to int
          
          int numOfVerse = lookup(surahValue +1);
          Serial.println(numOfVerse);
          
          for (int o = 0; o < numOfVerse; o++){
            chosenVerses[surahValue][o] = true;
          }
        }  
      }      
    } 
    else {
      
      Serial.println("Message does not contain '+'.");
      numSurah = 1;

      
      int verseIndex = 0;

      //Splitting SUrah from verses
      SplitResult seperator;

      if (lastMessage.indexOf(":") != -1) {
        seperator = splitMessage(lastMessage, ':');

        Serial.println("Splitting Surah from Verse:");
        for (int k = 0; k < seperator.count; k++) {
          seperator.parts[k].trim();
          Serial.println("Stored Part [" + String(k) + "]: " + seperator.parts[k]);
        }
        int surahValue = seperator.parts[0].toInt() -1;
        chosenSurahs[surahValue] = true;
        //SPLITTING VERSE SELECTION
        SplitResult msgVerses;

        if (seperator.parts[1].indexOf(",") != -1){
          msgVerses = splitMessage(seperator.parts[1], ',');
          Serial.println("Splitting selected verses:");
          for (int l = 0; l < msgVerses.count; l++) {
            msgVerses.parts[l].trim();
            Serial.println("Stored Part [" + String(l) + "]: " + msgVerses.parts[l]);
            //SPLITTING RANGE Selection
            SplitResult selection;
            if (msgVerses.parts[l].indexOf("-") != -1){
              Serial.println("Splitting selection:");
              selection = splitMessage(msgVerses.parts[l], '-');

              for (int m = 0; m < selection.count; m++) {
                selection.parts[m].trim();
                Serial.println("Stored Part [" + String(m) + "]: " + selection.parts[m]);

                //selection.parts[m].replace(" ", "");                  
              }
              int startVerse = selection.parts[0].toInt() -1;
              int endVerse = selection.parts[1].toInt() -1;
              int rangeVerse = endVerse - startVerse;

              for (int n = startVerse; n <= endVerse; n++) {
                //Serial.println(n);
                chosenVerses[surahValue][n] = true;
                verseIndex += 1;
              }
            }
            else{
              Serial.println("Non Selection");
              //msgVerses.parts[l].replace(" ", "");
              msgVerses.parts[l].trim();
              Serial.println(msgVerses.parts[l]);
              //Serial.println(chosenVerses[j][verseIndex]);
              int msgValue = msgVerses.parts[l].toInt() -1;
              chosenVerses[surahValue][msgValue] = true;
              verseIndex += 1;                
            }
          }
        }
        else{
          //msgVerses = seperator.parts[1];

          //SPLITTING RANGE Selection
          SplitResult selection;
          if (seperator.parts[1].indexOf("-") != -1){
            Serial.println("Splitting selection:");
            selection = splitMessage(seperator.parts[1], '-');

            for (int m = 0; m < selection.count; m++) {
              selection.parts[m].trim();
              Serial.println("Stored Part [" + String(m) + "]: " + selection.parts[m]);

              //selection.parts[m].replace(" ", "");                  
            }
            int startVerse = selection.parts[0].toInt() -1;
            int endVerse = selection.parts[1].toInt() -1;
            int rangeVerse = endVerse - startVerse;
            
            Serial.println("range");

            for (int n = startVerse; n <= endVerse; n++) {
              chosenVerses[surahValue][n] = true; 
              verseIndex += 1;
            }
          }
          else{
            Serial.println("Non Selection");
            //msgVerses.replace(" ", "");
            seperator.parts[1].trim();
            Serial.println(seperator.parts[1]);
            //Serial.println(chosenVerses[j][verseIndex]);
            int msgValue = seperator.parts[1].toInt() -1;
            chosenVerses[surahValue][msgValue] = true;
            verseIndex += 1;                
          }
        }
      }
      else{
        //SURAH WITH ALL VERSES ADDED
        Serial.println("all verses");
        //Serial.println(lastMessage);
        lastMessage.trim();
        int surahValue = lastMessage.toInt() -1;
        chosenSurahs[surahValue] = true;
        Serial.println(surahValue +1);
        int numOfVerse = lookup(surahValue +1);
        Serial.println(numOfVerse);

        for (int o = 0; o < numOfVerse; o++){
          chosenVerses[surahValue][o] = true;
        }

      }

      
    }
    
    for (int j = 0; j < 114; j++) {
      if (chosenSurahs[j]) {
        Serial.println("Surah " + String(j + 1));  // Surah index is 0-based, display as 1-based

    
        Serial.print("Selected Verses: ");
        for (int n = 0; n < 286; n++) {
          if (chosenVerses[j][n]) {
            Serial.print(n + 1); // verse numbers are 1-based
            Serial.print(" ");
          }
        }
        Serial.println();
      }
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
