#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"

const char* ssid = "";
const char* password = "";
const char* googleScriptURL = "";

// NTP server for RTC time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// pulse pin
const int pulsePin = 14;

volatile unsigned long pulseCount = 0;
volatile unsigned long lastInterruptTime = 0;

// debounce time (milliseconds)
const int debounceDelay = 20;

// send interval
unsigned long lastSend = 0;
const unsigned long sendInterval = 900000; // 10 minutes

// interrupt routine
void IRAM_ATTR pulseISR()
{
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > debounceDelay)
  {
    pulseCount++;
    lastInterruptTime = interruptTime;
  }
}

void connectWiFi()
{
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
}

String getTimeString()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
  {
    return "NoTime";
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

  return String(buffer);
}

void sendToGoogleSheets(unsigned long pulses)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    http.begin(googleScriptURL);
    http.addHeader("Content-Type", "application/json");

    String timestamp = getTimeString();

    String json = "{";
    json += "\"time\":\"" + timestamp + "\",";
    json += "\"pulse\":" + String(pulses);
    json += "}";

    int httpResponse = http.POST(json);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponse);

    http.end();
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(pulsePin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pulsePin), pulseISR, RISING);

  connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("RTC time synchronized");
}

void loop()
{
  if (millis() - lastSend > sendInterval)
  {
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    Serial.print("Pulses in 15 minutes: ");
    Serial.println(pulses);

    sendToGoogleSheets(pulses);

    lastSend = millis();
  }
}
