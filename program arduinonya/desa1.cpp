#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT Configuration
const char* ssid = "OWNER GATOT";
const char* password = "HARUSGEDE";
const char* mqtt_server = "maqiatto.com";
const int mqtt_port = 1883;
const char* mqtt_user = "mohamadkharizalfirdaus@gmail.com";
const char* mqtt_pass = "Rizal020305+";

WiFiClient espClient;
PubSubClient client(espClient);

// Local HC-SR04 sensor pins
const int trigPin = 5;
const int echoPin = 18;

// Rain sensor pin
const int rainPin = 19;

typedef struct struct_message {
  float distance;
  char rainStatus[32];
} struct_message;

struct_message receivedDataDesa2;
String desa1Data = "Waiting for data";
String desa2Data = "Waiting for data";
String rainStatus = "Waiting for data";

// Rain detection variables
enum RainState { NO_RAIN, LIGHT_RAIN, MODERATE_RAIN, HEAVY_RAIN };
RainState currentRainState = NO_RAIN;
unsigned long rainStartTime = 0;
unsigned long moderateThreshold = 10000;  // 10 seconds to moderate
unsigned long heavyThreshold = 20000;     // 20 seconds to heavy

void callback(char* topic, byte* payload, unsigned int length) {
  // Not needed for publisher
}

float readLocalDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 20000);
  return (duration == 0) ? -1 : (duration * 0.0343) / 2;
}

String detectRainStatus() {
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
    case LIGHT_RAIN: return "Light Rain";
    case MODERATE_RAIN: return "Moderate Rain";
    case HEAVY_RAIN: return "Heavy Rain";
    default: return "No Rain";
  }
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    retry++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Desa1", mqtt_user, mqtt_pass)) {
      Serial.println("Connected to MQTT");
      client.subscribe("mohamadkharizalfirdaus@gmail.com/desa1/hcsr04");
      client.subscribe("mohamadkharizalfirdaus@gmail.com/desa1/rain");
      client.subscribe("mohamadkharizalfirdaus@gmail.com/desa2/hcsr04");
      client.subscribe("mohamadkharizalfirdaus@gmail.com/desa2/rain");
    } else {
      delay(5000);
    }
  }
}

void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
  memcpy(&receivedDataDesa2, incomingData, sizeof(receivedDataDesa2));
  
  if (receivedDataDesa2.distance < 0) {
    desa2Data = "Invalid";
  } else {
    desa2Data = String(receivedDataDesa2.distance) + " cm";
  }
  
  Serial.print("Received from Desa 2 - Distance: ");
  Serial.println(desa2Data);
  Serial.print("Received from Desa 2 - Rain Status: ");
  Serial.println(receivedDataDesa2.rainStatus);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(rainPin, INPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("ESP1 Ready (Local Sensor + ESP-NOW Receiver)");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long lastPublish = 0;
  if (millis() - lastPublish >= 500) {
    lastPublish = millis();
    
    float localDistance = readLocalDistance();
    desa1Data = (localDistance < 0) ? "Invalid" : String(localDistance) + " cm";
    
    rainStatus = detectRainStatus();
    
    client.publish("mohamadkharizalfirdaus@gmail.com/desa1/hcsr04", desa1Data.c_str());
    client.publish("mohamadkharizalfirdaus@gmail.com/desa1/rain", rainStatus.c_str());
    client.publish("mohamadkharizalfirdaus@gmail.com/desa2/hcsr04", desa2Data.c_str());
    client.publish("mohamadkharizalfirdaus@gmail.com/desa2/rain", receivedDataDesa2.rainStatus);
    
    Serial.println("[Desa1] " + desa1Data + " | [Rain1] " + rainStatus + 
                  " | [Desa2] " + desa2Data + " | [Rain2] " + receivedDataDesa2.rainStatus);
  }
}