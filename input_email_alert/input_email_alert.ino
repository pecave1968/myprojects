/*
  ESP32 Input Email Alert
  ========================
  Sends an email when a monitored GPIO input goes HIGH,
  and again when it goes LOW.

  Dependencies (install via Arduino Library Manager):
    - ESP Mail Client  by Mobizt  (https://github.com/mobizt/ESP-Mail-Client)

  Hardware:
    - Connect your signal to INPUT_PIN (default GPIO 4)
    - Use a pull-down or pull-up resistor as appropriate for your circuit

  Configuration:
    - Fill in the #define values below before uploading
*/

#include <WiFi.h>
#include <ESP_Mail_Client.h>

// ─── WiFi Credentials ────────────────────────────────────────────────────────
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

// ─── SMTP / Email Settings ────────────────────────────────────────────────────
// Gmail example — for Gmail you MUST use an App Password (not your account password).
// Generate one at: https://myaccount.google.com/apppasswords
#define SMTP_HOST       "smtp.gmail.com"
#define SMTP_PORT       465           // 465 = SSL, 587 = STARTTLS
#define SENDER_EMAIL    ""
#define SENDER_PASSWORD "your_app_password"   // App Password, NOT your Gmail password
#define RECIPIENT_EMAIL ""
// ─── Input Pin ───────────────────────────────────────────────────────────────
#define INPUT_PIN       4             // GPIO pin to monitor
#define INPUT_MODE      INPUT_PULLDOWN  // INPUT_PULLDOWN or INPUT_PULLUP

// ─── Optional: debounce time (ms) ────────────────────────────────────────────
#define DEBOUNCE_MS     50

// ─── Globals ──────────────────────────────────────────────────────────────────
SMTPSession smtp;
bool lastState      = LOW;
bool stableState    = LOW;
unsigned long lastChangeTime = 0;

// ─── Forward Declarations ────────────────────────────────────────────────────
void sendEmail(const char* subject, const char* body);
void smtpCallback(SMTP_Status status);

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(INPUT_PIN, INPUT_MODE);

  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());

  // Read initial state so we don't false-trigger on boot
  stableState = (bool)digitalRead(INPUT_PIN);
  lastState   = stableState;
  Serial.printf("Initial pin state: %s\n", stableState ? "HIGH" : "LOW");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  bool currentRead = (bool)digitalRead(INPUT_PIN);

  // Debounce: only accept a new state after it has been stable for DEBOUNCE_MS
  if (currentRead != lastState) {
    lastChangeTime = millis();
    lastState = currentRead;
  }

  if ((millis() - lastChangeTime) >= DEBOUNCE_MS && currentRead != stableState) {
    stableState = currentRead;

    if (stableState == HIGH) {
      Serial.println("Input went HIGH — sending email...");
      sendEmail(
        "Alert: Input HIGH on ESP32",
        "GPIO input pin " STR(INPUT_PIN) " has gone HIGH.\n\nThis is an automated alert from your ESP32."
      );
    } else {
      Serial.println("Input went LOW — sending email...");
      sendEmail(
        "Alert: Input LOW on ESP32",
        "GPIO input pin " STR(INPUT_PIN) " has returned LOW.\n\nThis is an automated alert from your ESP32."
      );
    }
  }

  delay(10);
}

// ─── Send Email Helper ────────────────────────────────────────────────────────
void sendEmail(const char* subject, const char* body) {
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port      = SMTP_PORT;
  session.login.email      = SENDER_EMAIL;
  session.login.password   = SENDER_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name  = "ESP32 Alert";
  message.sender.email = SENDER_EMAIL;
  message.subject      = subject;
  message.addRecipient("Recipient", RECIPIENT_EMAIL);
  message.text.content = body;
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  smtp.debug(1);  // set to 0 to silence SMTP debug output
  smtp.callback(smtpCallback);

  if (!smtp.connect(&session)) {
    Serial.printf("SMTP connect error: %s\n", smtp.errorReason().c_str());
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.printf("Send error: %s\n", smtp.errorReason().c_str());
  }

  smtp.closeSession();
}

// ─── SMTP Status Callback ─────────────────────────────────────────────────────
void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
  if (status.success()) {
    Serial.println("Email sent successfully.");
  }
}

// Helper macro to stringify a #define value
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
