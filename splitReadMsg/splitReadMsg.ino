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

SplitResult splitByDelimiter(String message, char delimiter);

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

const int MAX_SURAHS = 114;
const int MAX_VERSES = 286;
bool chosenSurahs[MAX_SURAHS] = { false };       // true if Surah is selected
bool chosenVerses[MAX_SURAHS][MAX_VERSES] = { false };    // true if that specific verse is selected


void processComboMessage(String message);
void processSingleSurahMessage(String message);
void processSurahWithVerses(int surahIndex, String verses);
void processVerseSelections(int surahIndex, String versePart);
void processRange(int surahIndex, String rangeStr);
void markAllVersesInSurah(int surahIndex);
void printSelectedVerses();

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

  handleMessage();

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
SplitResult splitByDelimiter(String message, char delimiter) {
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

void handleMessage() {
  String lastMessage = getLastMessageFromDiscord();
  if (lastMessage == "") {
    Serial.println("Failed to fetch the last message.");
    return;
  }

  Serial.println("Last Message: " + lastMessage);

  if (lastMessage.indexOf('+') != -1) {
    processComboMessage(lastMessage);
  } else {
    processSingleSurahMessage(lastMessage);
  }

  printSelectedVerses();
}

void processComboMessage(String message) {
  SplitResult combos = splitByDelimiter(message, '+');
  for (int i = 0; i < combos.count; i++) {
    String part = combos.parts[i];
    part.trim();

    if (part.indexOf(':') != -1) {
      SplitResult sep = splitByDelimiter(part, ':');
      int surahIndex = sep.parts[0].toInt() - 1;
      chosenSurahs[surahIndex] = true;
      processSurahWithVerses(surahIndex, sep.parts[1]);
    } else {
      int surahIndex = part.toInt() - 1;
      chosenSurahs[surahIndex] = true;
      markAllVersesInSurah(surahIndex);
    }
  }
}

void processSingleSurahMessage(String message) {
  message.trim();
  if (message.indexOf(':') != -1) {
    SplitResult sep = splitByDelimiter(message, ':');
    int surahIndex = sep.parts[0].toInt() - 1;
    chosenSurahs[surahIndex] = true;
    processSurahWithVerses(surahIndex, sep.parts[1]);
  } else {
    int surahIndex = message.toInt() - 1;
    chosenSurahs[surahIndex] = true;
    markAllVersesInSurah(surahIndex);
  }
}

void processSurahWithVerses(int surahIndex, String verses) {
  if (verses.indexOf(',') != -1) {
    SplitResult parts = splitByDelimiter(verses, ',');
    for (int i = 0; i < parts.count; i++) {
      processVerseSelections(surahIndex, parts.parts[i]);
    }
  } else {
    processVerseSelections(surahIndex, verses);
  }
}

void processVerseSelections(int surahIndex, String versePart) {
  versePart.trim();
  if (versePart.indexOf('-') != -1) {
    processRange(surahIndex, versePart);
  } else {
    int verse = versePart.toInt() - 1;
    chosenVerses[surahIndex][verse] = true;
  }
}

void processRange(int surahIndex, String rangeStr) {
  SplitResult range = splitByDelimiter(rangeStr, '-');
  int start = range.parts[0].toInt() - 1;
  int end = range.parts[1].toInt() - 1;
  for (int i = start; i <= end; i++) {
    chosenVerses[surahIndex][i] = true;
  }
}

void markAllVersesInSurah(int surahIndex) {
  int numVerses = lookup(surahIndex + 1);
  for (int i = 0; i < numVerses; i++) {
    chosenVerses[surahIndex][i] = true;
  }
}

void printSelectedVerses() {
  for (int i = 0; i < MAX_SURAHS; i++) {
    if (chosenSurahs[i]) {
      Serial.println("Surah " + String(i + 1));
      Serial.print("Selected Verses: ");
      for (int j = 0; j < MAX_VERSES; j++) {
        if (chosenVerses[i][j]) {
          Serial.print(j + 1);
          Serial.print(" ");
        }
      }
      Serial.println();
    }
  }
}
