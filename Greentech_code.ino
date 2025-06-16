#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "KABU";
const char* password = ""; // Add your WiFi password here

// MQTT broker IP and port
const char* mqtt_server = "10.1.19.51";
const uint16_t mqtt_port = 1883;

// Sensor and control pin configuration
#define DHTPIN 4
#define DHTTYPE DHT11
#define FAN_PIN 5
#define PUMP_PIN 18
#define MOISTURE_PIN 34

// Threshold constants
const float TEMP_ON_THRESHOLD = 30.0;     // Temperature to turn fan ON
const float TEMP_OFF_THRESHOLD = 28.0;    // Temperature to turn fan OFF
const float HUM_THRESHOLD = 60.0;         // Humidity threshold to turn fan ON
const int MOISTURE_THRESHOLD = 2000;      // Soil moisture threshold; lower means dryer soil

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

// States for mode and manual control
bool autoMode = true;          // true = ESP decides automatically
bool pumpManual = false;       // Whether pump is manually controlled
bool fanManual = false;        // Whether fan is manually controlled
bool lastAutoMode = autoMode;  // Track mode change for serial print
bool fanState = false;         // Current fan state to implement hysteresis

unsigned long lastMsg = 0;
const long interval = 5000;    // Sensor publish interval (ms)

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

  if (String(topic) == "greenhouse/irrigation") {
    pumpManual = true;
    if (message == "ON") {
      digitalWrite(PUMP_PIN, HIGH);
      Serial.println("Pump ON (manual)");
    } else if (message == "OFF") {
      digitalWrite(PUMP_PIN, LOW);
      Serial.println("Pump OFF (manual)");
    }
  } else if (String(topic) == "greenhouse/ventilation") {
    fanManual = true;
    if (message == "OPEN") {
      digitalWrite(FAN_PIN, HIGH);
      fanState = true;
      Serial.println("Fan OPEN (manual)");
    } else if (message == "CLOSE") {
      digitalWrite(FAN_PIN, LOW);
      fanState = false;
      Serial.println("Fan CLOSE (manual)");
    }
  } else if (String(topic) == "greenhouse/mode") {
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
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected");
      // Subscribe to control topics
      client.subscribe("greenhouse/irrigation");
      client.subscribe("greenhouse/ventilation");
      client.subscribe("greenhouse/mode");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
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

  // Read sensors
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int moisture = analogRead(MOISTURE_PIN);

  // Validate sensor readings
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(1000);
    return; // Skip rest of this loop iteration if sensor read failed
  }

  // Soil dryness check
  bool isSoilDry = (moisture < MOISTURE_THRESHOLD); // Lower value means drier soil

  // Debug sensor readings
  Serial.print("Temp: "); Serial.print(temp); Serial.print(" °C, ");
  Serial.print("Humidity: "); Serial.print(hum); Serial.print(" %, ");
  Serial.print("Soil Moisture: "); Serial.println(moisture);

  // Detect mode change for serial output
  if (autoMode != lastAutoMode) {
    if (autoMode) {
      Serial.println("Switched to AUTO mode");
    } else {
      Serial.println("Switched to MANUAL mode");
    }
    lastAutoMode = autoMode;
  }

  // Control logic
  if (autoMode) {
    // AUTO mode: ESP controls based on sensor data

    // Fan control hysteresis
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

    // Soil moisture control
    digitalWrite(PUMP_PIN, isSoilDry ? HIGH : LOW);
    Serial.print("Pump ");
    Serial.println(isSoilDry ? "ON (auto)" : "OFF (auto)");

  } else {
    // MANUAL mode: respect manual commands
    // If manual control not triggered, devices stay as-is
    if (!pumpManual) {
      // Optional: define default pump state or keep last known state
    }
    if (!fanManual) {
      // Optional: define default fan state or keep last known state
    }
    // Manual commands are handled via MQTT callbacks
  }

  // Periodic sensor data publish
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;

    client.publish("greenhouse/temperature", String(temp).c_str());
    client.publish("greenhouse/humidity", String(hum).c_str());
    client.publish("greenhouse/soilMoisture", String(moisture).c_str());

    Serial.println("===== Sensor Data Published =====");
  }

  delay(100);
}

