#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// === WiFi credentials ===
const char* ssid = "VM9999189";
const char* password = "nz6jjTknJxdy";

// === Google Apps Script Web App URL ===
const char* scriptUrl = "https://script.google.com/macros/s/AKfycbxWjXFKMF0iqrURvOqjzVPGW45S4AbZP4NRvlOj0z2TqG-FaEzm1q3hlnazC5KwqUG7/exec";

// === DS18B20 setup ===
#define ONE_WIRE_BUS 4  // GPIO4 (D4)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  sensors.begin();
}

void loop() {
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(scriptUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"temperature\": " + String(temperatureC, 2) + "}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("Data sent. HTTP %d\n", httpResponseCode);
    } else {
      Serial.printf("Error sending data: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(900000); // send every 15 mins
  }