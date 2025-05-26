#include <DHT.h>

#define DHTPIN A0
#define DHTTYPE DHT22
#define TRIG_PIN A3
#define ECHO_PIN A2

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  dht.begin();
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float distance = readUltrasonic();

  Serial.print(temperature); Serial.print(",");
  Serial.print(humidity); Serial.print(",");
  Serial.println(distance);

  delay(2000);
}


float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // timeout: 30 ms
  if (duration == 0) return -1;  // No echo received
  return duration * 0.0343 / 2;  // Convert to cm
}
