#include <esp_now.h>
#include <WiFi.h>
#include <string.h>

const int trigPin = 5;
const int echoPin = 18;
const int rainPin = 19;

typedef struct struct_message {
  float distance;
  char rainStatus[32];
} struct_message;

struct_message myData;
uint8_t broadcastAddress[] = {0xEC, 0x64, 0xC9, 0x84, 0xF6, 0xA0};

unsigned long previousMillis = 0;
const unsigned long interval = 100;

// Rain detection variables
enum RainState { NO_RAIN, LIGHT_RAIN, MODERATE_RAIN, HEAVY_RAIN };
RainState currentRainState = NO_RAIN;
unsigned long rainStartTime = 0;
const unsigned long moderateThreshold = 10000;  // 10 seconds to moderate
const unsigned long heavyThreshold = 20000;     // 20 seconds to heavy

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(rainPin, INPUT);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    ESP.restart();
  }

  Serial.println("Sender ready");
}

void detectRainStatus() {
  int rainState = digitalRead(rainPin);
  
  if (rainState == LOW) {  // Water detected
    if (currentRainState == NO_RAIN) {
      currentRainState = LIGHT_RAIN;
      rainStartTime = millis();
    } 
    else {
      unsigned long elapsed = millis() - rainStartTime;
      
      if (currentRainState == LIGHT_RAIN && elapsed >= moderateThreshold) {
        currentRainState = MODERATE_RAIN;
      }
      else if (currentRainState == MODERATE_RAIN && elapsed >= heavyThreshold) {
        currentRainState = HEAVY_RAIN;
      }
    }
  } 
  else {  // No water detected
    currentRainState = NO_RAIN;
    rainStartTime = 0;
  }

  switch(currentRainState) {
    case LIGHT_RAIN: strcpy(myData.rainStatus, "Light Rain"); break;
    case MODERATE_RAIN: strcpy(myData.rainStatus, "Moderate Rain"); break;
    case HEAVY_RAIN: strcpy(myData.rainStatus, "Heavy Rain"); break;
    default: strcpy(myData.rainStatus, "No Rain"); break;
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read distance sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 15000);
    myData.distance = (duration == 0) ? -1.0 : (duration * 0.0343) / 2;

    // Detect rain status
    detectRainStatus();

    // Send data via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
      Serial.print("Sent distance: ");
      Serial.println((myData.distance < 0) ? "Invalid" : String(myData.distance).c_str());
      Serial.print("Rain Status: ");
      Serial.println(myData.rainStatus);
    } else {
      Serial.println("Error sending data");
    }
  }
}