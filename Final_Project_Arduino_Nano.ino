#include <WiFiNINA.h>
#include <PubSubClient.h>

// Wi-Fi credentials 
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// HiveMQ Cloud credentials 
const char* mqtt_server = "YOUR_MQTT_BROKER_URL";
const int mqtt_port = 8883;
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_pass = "YOUR_MQTT_PASSWORD";

WiFiSSLClient wifiClient;
PubSubClient client(wifiClient);

// Previous alert states
bool tempAlertActive = false;
bool humAlertActive = false;
bool distAlertActive = false;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }

  if (!client.connected()) {
    Serial.println("MQTT disconnected! Reconnecting...");
    connectMQTT();
  }

  client.loop();

  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    data.trim();
    data.replace("\r", "");

    Serial.println("Received from Uno: " + data);

    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);

    if (firstComma == -1 || secondComma == -1) {
      Serial.println("Data format error. Skipping.");
      return;
    }

    float temp = data.substring(0, firstComma).toFloat();
    float hum = data.substring(firstComma + 1, secondComma).toFloat();
    float dist = data.substring(secondComma + 1).toFloat();

    Serial.print("Parsed: ");
    Serial.print(temp); Serial.print(" °C, ");
    Serial.print(hum); Serial.print(" %, ");
    Serial.print(dist); Serial.println(" cm");

    // Publish regular sensor data
    String payload = String("{\"temperature\":") + temp +
                     ",\"humidity\":" + hum +
                     ",\"distance\":" + dist + "}";

    if (client.publish("iot/home/sensors", payload.c_str())) {
      Serial.println("✅ Sensor data published");
    } else {
      Serial.println("❌ Failed to publish sensor data");
    }

    // Check if alerts should be sent (only if state has changed)
    bool tempOutOfRange = (temp < 10 || temp > 30);
    bool humOutOfRange = (hum > 80);
    bool distOutOfRange = (dist < 15);

    // Temperature alert
    if (tempOutOfRange && !tempAlertActive) {
      publishAlert("temperature", temp, hum, dist);
      tempAlertActive = true;
    } else if (!tempOutOfRange && tempAlertActive) {
      Serial.println("✅ Temperature back to normal.");
      tempAlertActive = false;
    }

    // Humidity alert
    if (humOutOfRange && !humAlertActive) {
      publishAlert("humidity", temp, hum, dist);
      humAlertActive = true;
    } else if (!humOutOfRange && humAlertActive) {
      Serial.println("✅ Humidity back to normal.");
      humAlertActive = false;
    }

    // Distance alertÍ
    if (distOutOfRange && !distAlertActive) {
      publishAlert("distance", temp, hum, dist);
      distAlertActive = true;
    } else if (!distOutOfRange && distAlertActive) {
      Serial.println("✅ Distance back to normal.");
      distAlertActive = false;
    }
  }
}

void publishAlert(const String& type, float temp, float hum, float dist) {
  String reason = "";
  if (type == "temperature") {
    reason = (temp < 10) ? "Temperature below 10°C" : "Temperature above 30°C";
  } else if (type == "humidity") {
    reason = "Humidity above 80%";
  } else if (type == "distance") {
    reason = "Distance below 15 cm";
  }

  String alertPayload = String("{\"alert\":true,\"type\":\"") + type +
                        "\",\"reason\":\"" + reason +
                        "\",\"temperature\":" + temp +
                        ",\"humidity\":" + hum +
                        ",\"distance\":" + dist + "}";

  if (client.publish("iot/home/alerts", alertPayload.c_str())) {
    Serial.println("🚨 Alert published for " + type + ": " + reason);
  } else {
    Serial.println("❌ Failed to publish alert for " + type);
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP address: " + WiFi.localIP().toString());
}

void connectMQTT() {
  Serial.print("Connecting to MQTT broker...");
  while (!client.connected()) {
    if (client.connect("NanoClient", mqtt_user, mqtt_pass)) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 2 seconds...");
      delay(2000);
    }
  }
}
