#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "LOTOBO";
const char* password = "Kmtgrfn@2024"; // Your WiFi password

// MQTT broker IP and port
const char* mqtt_server = " 192.168.1.132";
const uint16_t mqtt_port = 1883;

// MQTT Topics
const char* TOPIC_IRRIGATION = "greenhouse/irrigation";
const char* TOPIC_VENTILATION = "greenhouse/ventilation";
const char* TOPIC_MODE = "greenhouse/mode";
const char* TOPIC_TEMPERATURE = "greenhouse/temperature";
const char* TOPIC_HUMIDITY = "greenhouse/humidity";
const char* TOPIC_SOIL_MOISTURE = "greenhouse/soilMoisturePercent";

// Sensor and control pin configuration
#define DHTPIN 4
#define DHTTYPE DHT11
#define FAN_PIN 5
#define PUMP_PIN 18
#define MOISTURE_PIN 34

// Threshold constants
const float TEMP_ON_THRESHOLD = 30.0;    
const float TEMP_OFF_THRESHOLD = 28.0;   
const float HUM_THRESHOLD = 60.0;        

// Soil moisture thresholds (adjust based on your sensor)
const int MOISTURE_DRY = 3500;   // Higher ADC = dry
const int MOISTURE_WET = 1500;   // Lower ADC = wet

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

// States
bool autoMode = true;
bool pumpManual = false;
bool fanManual = false;
bool lastAutoMode = autoMode;
bool fanState = false;

unsigned long lastMsg = 0;
const long interval = 5000;    // Publish interval
unsigned long lastPrintTime = 0;
const long printInterval = 60000;

void setup_wifi() {
  delay(100);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("] = ");
  Serial.println(message);

  if (String(topic) == TOPIC_IRRIGATION) {
    pumpManual = true;
    if (message == "ON") {
      digitalWrite(PUMP_PIN, HIGH);
      Serial.println("Pump ON (manual)");
    } else if (message == "OFF") {
      digitalWrite(PUMP_PIN, LOW);
      Serial.println("Pump OFF (manual)");
    }
  } else if (String(topic) == TOPIC_VENTILATION) {
    fanManual = true;
    if (message == "OPEN") {
      digitalWrite(FAN_PIN, HIGH);
      fanState = true;
      Serial.println("Fan ON (manual)");
    } else if (message == "CLOSE") {
      digitalWrite(FAN_PIN, LOW);
      fanState = false;
      Serial.println("Fan OFF (manual)");
    }
  } else if (String(topic) == TOPIC_MODE) {
    if (message == "AUTO") {
      autoMode = true;
      pumpManual = false;
      fanManual = false;
      Serial.println("Switched to AUTO mode");
    } else if (message == "MANUAL") {
      autoMode = false;
      Serial.println("Switched to MANUAL mode");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String((uint64_t)ESP.getEfuseMac(), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected");
      client.subscribe(TOPIC_IRRIGATION);
      client.subscribe(TOPIC_VENTILATION);
      client.subscribe(TOPIC_MODE);
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Fix: Map dry (high value) to 0% and wet (low value) to 100%
int getSoilMoisturePercent(int raw) {
  raw = constrain(raw, MOISTURE_WET, MOISTURE_DRY);
  int percent = map(raw, MOISTURE_DRY, MOISTURE_WET, 0, 100); // Reverse mapping
  return constrain(percent, 0, 100);
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int rawMoisture = analogRead(MOISTURE_PIN);
  int moisturePercent = getSoilMoisturePercent(rawMoisture);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(1000);
    return;
  }

  if (autoMode != lastAutoMode) {
    Serial.println(autoMode ? "Switched to AUTO mode" : "Switched to MANUAL mode");
    lastAutoMode = autoMode;
  }

  if (autoMode) {
    if ((temp > TEMP_ON_THRESHOLD) || (hum > HUM_THRESHOLD)) {
      if (!fanState) {
        digitalWrite(FAN_PIN, HIGH);
        fanState = true;
        Serial.println("Fan ON (auto)");
      }
    } else if ((temp < TEMP_OFF_THRESHOLD) && (hum < HUM_THRESHOLD)) {
      if (fanState) {
        digitalWrite(FAN_PIN, LOW);
        fanState = false;
        Serial.println("Fan OFF (auto)");
      }
    }

    if (!pumpManual) {
      if (moisturePercent < 40) {
        digitalWrite(PUMP_PIN, HIGH);
        Serial.println("Pump ON (auto - soil dry)");
      } else {
        digitalWrite(PUMP_PIN, LOW);
        Serial.println("Pump OFF (auto - soil moist)");
      }
    }
  }

  // Debug prints
  Serial.print("Raw Moisture ADC: ");
  Serial.print(rawMoisture);
  Serial.print(" -> Moisture: ");
  Serial.print(moisturePercent);
  Serial.println(" %");

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    client.publish(TOPIC_TEMPERATURE, String(temp, 1).c_str());
    client.publish(TOPIC_HUMIDITY, String(hum, 1).c_str());
    client.publish(TOPIC_SOIL_MOISTURE, String(moisturePercent).c_str());
    Serial.println("===== Sensor Data Published =====");
  }

  if (now - lastPrintTime >= printInterval) {
    lastPrintTime = now;
    Serial.println("---------- Sensor Readings ----------");
    Serial.printf("Temperature: %.1f °C\n", temp);
    Serial.printf("Humidity: %.1f %%\n", hum);
    Serial.printf("Soil Moisture: %d %%\n", moisturePercent);
    Serial.println("------------------------------------");
  }

  delay(100);
}