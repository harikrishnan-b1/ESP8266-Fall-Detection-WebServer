#include "DFRobot_HumanDetection.h"
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// --- Sensor ---
SoftwareSerial radarSerial(4, 5); // RX = GPIO4 (D2), TX = GPIO5 (D1)
DFRobot_HumanDetection hu(&radarSerial);

// --- Wi-Fi Credentials ---
const char* ssid = "anuradha";
const char* password = "1234567890";

// --- Web Server ---
ESP8266WebServer server(80);

// --- Sensor Data (global) ---
int presence = 0;
int motion = 0;
int fall = 0;
int intensity = 0;

// Helper: Motion description
String getMotionStr(int m) {
  switch (m) {
    case 0: return "None";
    case 1: return "Still (breathing)";
    case 2: return "Active movement";
    default: return "Unknown";
  }
}

// Helper: Fall status
String getFallStr(int f) {
  return (f == 1) ? "⚠ FALL DETECTED!" : "OK";
}

// Web page handler
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>C1001 IoT</title>";
  html += "<style>body{font-family:Arial,sans-serif;text-align:center;background:#f9f9f9;padding:20px;}";
  html += "h2{color:#333;} .card{background:white;padding:15px;margin:10px;border-radius:10px;box-shadow:0 2px 5px #ccc;}";
  html += ".alert{color:red;font-weight:bold;}</style>";
  html += "</head><body>";

  html += "<h2>🏠 C1001 Fall & Motion Monitor</h2>";

  html += "<div class='card'>";
  html += "<b>👤 Presence:</b> ";
  html += (presence ? "Detected" : "None");
  html += "</div>";

  html += "<div class='card'>";
  html += "<b>🏃 Motion:</b> ";
  html += getMotionStr(motion);
  html += "</div>";

  html += "<div class='card'>";
  html += "<b>📊 Movement Level:</b> ";
  html += String(intensity);
  html += "</div>";

  html += "<div class='card ";
  if (fall) html += "alert";
  html += "'>";
  html += "<b>🚨 Fall Status:</b> ";
  html += getFallStr(fall);
  html += "</div>";

  html += "<p><i>Auto-refresh enabled</i></p>";
  html += "<script>setTimeout(()=>location.reload(),2000);</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  radarSerial.begin(115200);

  Serial.println("\nInitializing C1001 sensor...");
  while (hu.begin() != 0) {
    Serial.println("Sensor init failed. Retrying...");
    delay(1000);
  }

  // Switch to Fall Detection Mode
  while (hu.configWorkMode(hu.eFallingMode) != 0) {
    Serial.println("Failed to set fall mode. Retrying...");
    delay(1000);
  }

  // Configure fall parameters (as per PDF)
  hu.configLEDLight(hu.eHPLed, 1);
  hu.configLEDLight(hu.eFALLLed, 1);
  hu.dmInstallHeight(270);               // 2.7 m
  hu.dmFallTime(5);                      // 5 sec confirmation
  hu.dmUnmannedTime(1);
  hu.dmFallConfig(hu.eResidenceTime, 200);
  hu.dmFallConfig(hu.eFallSensitivityC, 3);
  hu.sensorRet(); // Apply settings

  Serial.println("✅ Sensor ready in Fall Detection Mode");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi connected");
  Serial.print("👉 Open on phone: http://");
  Serial.println(WiFi.localIP());

  // Start server
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  // Read sensor data
  presence = hu.smHumanData(hu.eHumanPresence);
  motion = hu.smHumanData(hu.eHumanMovement);
  fall = hu.getFallData(hu.eFallState);
  intensity = hu.smHumanData(hu.eHumanMovingRange);

  // Optional: print to Serial
  Serial.printf("Presence: %d | Motion: %d | Fall: %d | Level: %d\n", presence, motion, fall, intensity);

  // Handle web requests
  server.handleClient();

  delay(100); // Keep responsive
}