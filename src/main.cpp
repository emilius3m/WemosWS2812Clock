
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <time.h>
#include <EEPROM.h>

#define PIN D1                 // Pin connected to WS2812 data pin
#define NUM_LEDS 60            // Number of LEDs in the ring
Adafruit_NeoPixel ring(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);
WiFiManager wm;

char ntpServer[64] = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.google.com";
const char* NTP_SERVER_3 = "time.cloudflare.com";
const char* TZ_INFO = "CET-1CEST,M3.5.0/2,M10.5.0/3"; // Europe/Rome
bool timeSynced = false;
bool wifiConnectedHandled = false;
unsigned long lastNtpRetryMs = 0;

struct SavedSettings {
  uint32_t magic;
  uint32_t colorQuadrants;
  uint32_t colorHourHand;
  uint32_t colorMinuteHand;
  uint32_t colorSecondHand;
  uint8_t brightness;
  uint8_t showQuadrants;
  char ntpServer[64];
};

const uint32_t SETTINGS_MAGIC = 0xC10C2026;
const int EEPROM_SIZE = 512;

// Forward declarations
void rotatingRingAnimation();
void pulsatingGlowAnimation();
void progressBarAnimation();
void wifiSearchingAnimation();
void wifiConnectingAnimation();
void wifiConnectedAnimation();
void wifiFailedAnimation();
void displayClock();
void handleRoot();
void handleUpdate();
void handleTestAnimation();
uint32_t hexToColor(String hex);
String colorToHex(uint32_t color);
uint32_t applyGammaCorrection(uint32_t color);
bool syncTimeWithNTP();
void loadSettings();
void saveSettings();

// Default settings
uint32_t colorQuadrants = ring.Color(255, 255, 255);   // Hour markers
uint32_t colorHourHand = ring.Color(255, 0, 0);        // Hour hand
uint32_t colorMinuteHand = ring.Color(255, 255, 255);  // Minute hand
uint32_t colorSecondHand = ring.Color(0, 0, 255);      // Second hand
bool showQuadrants = true;
int brightness = 50;

void setup() {
  Serial.begin(9600);

  EEPROM.begin(EEPROM_SIZE);
  loadSettings();

  // Initialize NeoPixel Ring
  ring.begin();
  ring.setBrightness(brightness);
  ring.show();


  // Play startup animations
  rotatingRingAnimation();
  pulsatingGlowAnimation();
  progressBarAnimation();
  // Wi-Fi setup with captive portal (no hardcoded SSID/password)
  // Non-blocking mode keeps LED animations alive while portal is active.
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(120); // Timeout after 3 minutes if not configured

  Serial.println("Starting Wi-Fi auto connect...");
  bool connected = wm.autoConnect("Clock-Setup");
  if (connected) {
    Serial.println("Wi-Fi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    wifiConnectedAnimation();
    timeSynced = syncTimeWithNTP();
    wifiConnectedHandled = true;
  } else {
    Serial.println("Starting Web Portal (non-blocking)");
  }

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.on("/testAnimation", handleTestAnimation);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  wm.process();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnectedHandled) {
      Serial.println("Wi-Fi connected");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      wifiConnectedAnimation();
      timeSynced = syncTimeWithNTP();
      wifiConnectedHandled = true;
    }

    if (!timeSynced && (millis() - lastNtpRetryMs > 30000UL)) {
      lastNtpRetryMs = millis();
      timeSynced = syncTimeWithNTP();
    }

    displayClock();
  } else {
    wifiSearchingAnimation();
  }

  server.handleClient();
}

void displayClock() {
  ring.clear();

  time_t nowEpoch = time(nullptr);
  if (nowEpoch < 100000) {
    ring.show();
    delay(1000);
    return;
  }

  struct tm now;
  localtime_r(&nowEpoch, &now);

  // Mark the hour positions
  if (showQuadrants) {
    for (int i = 0; i < 12; i++) {
      ring.setPixelColor(i * 5, applyGammaCorrection(colorQuadrants));
    }
  }

  // Calculate positions
  int secondPos = (now.tm_sec % 60) * NUM_LEDS / 60;
  int minutePos = (now.tm_min % 60) * NUM_LEDS / 60;
  int hourPos = ((now.tm_hour % 12) * NUM_LEDS) / 12;

  // Set hands
  ring.setPixelColor(secondPos, applyGammaCorrection(colorSecondHand));
  ring.setPixelColor(minutePos, applyGammaCorrection(colorMinuteHand));
  ring.setPixelColor((hourPos - 1 + NUM_LEDS) % NUM_LEDS, applyGammaCorrection(colorHourHand));
  ring.setPixelColor(hourPos, applyGammaCorrection(colorHourHand));
  ring.setPixelColor((hourPos + 1) % NUM_LEDS, applyGammaCorrection(colorHourHand));

  ring.show();
  delay(1000); // Update every second
}

void handleRoot() {
  time_t nowEpoch = time(nullptr);
  struct tm now;
  localtime_r(&nowEpoch, &now);

  String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:16px;}";
  html += ".card{max-width:720px;margin:0 auto;background:#111827;border:1px solid #334155;border-radius:12px;padding:18px;}";
  html += "h1{margin:0 0 8px 0;font-size:22px;}h2{margin:20px 0 10px 0;font-size:16px;color:#93c5fd;}";
  html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;}@media(max-width:680px){.grid{grid-template-columns:1fr;}}";
  html += "label{font-size:13px;color:#cbd5e1;display:block;margin-bottom:6px;}";
  html += "input[type='color'],input[type='number'],input[type='text']{width:100%;height:40px;border-radius:8px;border:1px solid #475569;background:#0b1220;color:#e2e8f0;padding:0 10px;box-sizing:border-box;}";
  html += ".row{margin-bottom:12px;} .check{display:flex;align-items:center;gap:8px;margin:12px 0;}";
  html += "button{background:#2563eb;color:white;border:none;border-radius:8px;padding:10px 14px;font-weight:600;cursor:pointer;}";
  html += "small{color:#94a3b8;} .status{margin:8px 0 14px 0;padding:10px;border-radius:8px;background:#0b1220;border:1px solid #334155;}";
  html += "</style></head><body><div class='card'>";
  html += "<h1>WS2812 LED Ring Clock</h1>";
  html += "<div class='status'>";
  if (nowEpoch < 100000) {
    html += "Current Time: not synced (NTP)<br>";
  } else {
  html += "Current Time: " + String(now.tm_hour) + ":" + String(now.tm_min) + ":" + String(now.tm_sec) + "<br>";
  }
  html += "IP: " + WiFi.localIP().toString() + "<br>";
  html += "NTP: " + String(ntpServer);
  html += "</div>";

  html += "<h2>Animation Test</h2>";
  html += "<form action='/testAnimation' method='POST'>";
  html += "<div class='row'><label>Select animation</label>";
  html += "<select name='animation' style='width:100%;height:40px;border-radius:8px;border:1px solid #475569;background:#0b1220;color:#e2e8f0;padding:0 10px;box-sizing:border-box;'>";
  html += "<option value='rotating'>Rotating Ring</option>";
  html += "<option value='pulsating'>Pulsating Glow</option>";
  html += "<option value='progress'>Progress Bar</option>";
  html += "<option value='wifiSearching'>WiFi Searching</option>";
  html += "<option value='wifiConnecting'>WiFi Connecting</option>";
  html += "<option value='wifiConnected'>WiFi Connected</option>";
  html += "<option value='wifiFailed'>WiFi Failed</option>";
  html += "</select></div>";
  html += "<button type='submit'>Run Animation</button>";
  html += "</form>";

  html += "<form action='/update' method='POST'>";
  html += "<h2>LED Colors</h2><div class='grid'>";
  html += "<div class='row'><label>Quadrants</label><input type='color' name='quadrantsColor' value='" + colorToHex(colorQuadrants) + "'></div>";
  html += "<div class='row'><label>Hour Hand</label><input type='color' name='hourHandColor' value='" + colorToHex(colorHourHand) + "'></div>";
  html += "<div class='row'><label>Minute Hand</label><input type='color' name='minuteHandColor' value='" + colorToHex(colorMinuteHand) + "'></div>";
  html += "<div class='row'><label>Second Hand</label><input type='color' name='secondHandColor' value='" + colorToHex(colorSecondHand) + "'></div>";
  html += "</div>";
  html += "<h2>Display</h2>";
  html += "<div class='row'><label>Brightness (0-255)</label><input type='number' min='0' max='255' name='brightness' value='" + String(brightness) + "'></div>";
  html += "<label class='check'><input type='checkbox' name='showQuadrants' value='1'";
  if (showQuadrants) {
    html += " checked";
  }
  html += "> Show Quadrants</label>";
  html += "<h2>Time Sync</h2>";
  html += "<div class='row'><label>NTP Server</label><input type='text' name='ntpServer' maxlength='63' value='" + String(ntpServer) + "'></div>";
  html += "<small>Esempio: pool.ntp.org, time.google.com</small><br><br>";
  html += "<button type='submit'>Save Settings</button>";
  html += "</form>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.hasArg("quadrantsColor")) {
    colorQuadrants = hexToColor(server.arg("quadrantsColor"));
  }
  if (server.hasArg("hourHandColor")) {
    colorHourHand = hexToColor(server.arg("hourHandColor"));
  }
  if (server.hasArg("minuteHandColor")) {
    colorMinuteHand = hexToColor(server.arg("minuteHandColor"));
  }
  if (server.hasArg("secondHandColor")) {
    colorSecondHand = hexToColor(server.arg("secondHandColor"));
  }
  if (server.hasArg("brightness")) {
    brightness = server.arg("brightness").toInt();
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    ring.setBrightness(brightness);
  }
  showQuadrants = server.hasArg("showQuadrants");
  if (server.hasArg("ntpServer")) {
    String inputNtp = server.arg("ntpServer");
    inputNtp.trim();
    if (inputNtp.length() > 0 && inputNtp.length() < (int)sizeof(ntpServer)) {
      inputNtp.toCharArray(ntpServer, sizeof(ntpServer));
      timeSynced = syncTimeWithNTP();
    }
  }
  saveSettings();

  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "Updated");
}

void handleTestAnimation() {
  if (!server.hasArg("animation")) {
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "No animation selected");
    return;
  }

  String animation = server.arg("animation");

  if (animation == "rotating") {
    rotatingRingAnimation();
  } else if (animation == "pulsating") {
    pulsatingGlowAnimation();
  } else if (animation == "progress") {
    progressBarAnimation();
  } else if (animation == "wifiSearching") {
    for (int i = 0; i < NUM_LEDS; i++) {
      wifiSearchingAnimation();
    }
  } else if (animation == "wifiConnecting") {
    for (int i = 0; i < 20; i++) {
      wifiConnectingAnimation();
    }
  } else if (animation == "wifiConnected") {
    wifiConnectedAnimation();
  } else if (animation == "wifiFailed") {
    wifiFailedAnimation();
  }

  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "Animation executed");
}

uint32_t hexToColor(String hex) {
  long number = strtol(&hex[1], NULL, 16);
  return ring.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
}

String colorToHex(uint32_t color) {
  char hex[8];
  snprintf(hex, sizeof(hex), "#%02X%02X%02X", (uint8_t)((color >> 16) & 0xFF), (uint8_t)((color >> 8) & 0xFF), (uint8_t)(color & 0xFF));
  return String(hex);
}

uint32_t applyGammaCorrection(uint32_t color) {
  float gamma = 2.8; // Gamma value for correction
  uint8_t r = pow(((color >> 16) & 0xFF) / 255.0, gamma) * 255.0;
  uint8_t g = pow(((color >> 8) & 0xFF) / 255.0, gamma) * 255.0;
  uint8_t b = pow((color & 0xFF) / 255.0, gamma) * 255.0;
  return ring.Color(r, g, b);
}

bool syncTimeWithNTP() {
  Serial.println("Syncing time with NTP...");
  configTime(0, 0, ntpServer, NTP_SERVER_2, NTP_SERVER_3);
  setenv("TZ", TZ_INFO, 1);
  tzset();

  for (int i = 0; i < 60; i++) {
    time_t now = time(nullptr);
    if (now > 100000) {
      Serial.println("NTP sync successful");
      return true;
    }
    delay(250);
  }

  Serial.println("NTP sync failed");
  return false;
}

void loadSettings() {
  SavedSettings s;
  EEPROM.get(0, s);

  if (s.magic != SETTINGS_MAGIC) {
    Serial.println("No saved settings found, using defaults");
    return;
  }

  colorQuadrants = s.colorQuadrants;
  colorHourHand = s.colorHourHand;
  colorMinuteHand = s.colorMinuteHand;
  colorSecondHand = s.colorSecondHand;
  brightness = s.brightness;
  if (brightness > 255) brightness = 255;
  showQuadrants = (s.showQuadrants != 0);
  if (strlen(s.ntpServer) > 0 && strlen(s.ntpServer) < sizeof(ntpServer)) {
    strncpy(ntpServer, s.ntpServer, sizeof(ntpServer));
    ntpServer[sizeof(ntpServer) - 1] = '\0';
  }

  Serial.println("Settings loaded from EEPROM");
}

void saveSettings() {
  SavedSettings s;
  s.magic = SETTINGS_MAGIC;
  s.colorQuadrants = colorQuadrants;
  s.colorHourHand = colorHourHand;
  s.colorMinuteHand = colorMinuteHand;
  s.colorSecondHand = colorSecondHand;
  s.brightness = (uint8_t)brightness;
  s.showQuadrants = showQuadrants ? 1 : 0;
  strncpy(s.ntpServer, ntpServer, sizeof(s.ntpServer));
  s.ntpServer[sizeof(s.ntpServer) - 1] = '\0';

  EEPROM.put(0, s);
  EEPROM.commit();
  Serial.println("Settings saved to EEPROM");
}






void rotatingRingAnimation() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.clear();
    ring.setPixelColor(i, ring.Color(0, 0, 255)); // Blue
    ring.show();
    delay(50); // Adjust speed as needed
  }
}

void pulsatingGlowAnimation() {
  for (int brightness = 0; brightness <= 255; brightness += 5) {
    ring.setBrightness(brightness);
    ring.fill(ring.Color(0, 0, 255)); // Blue
    ring.show();
    delay(20);
  }
  for (int brightness = 255; brightness >= 0; brightness -= 5) {
    ring.setBrightness(brightness);
    ring.fill(ring.Color(0, 0, 255)); // Blue
    ring.show();
    delay(20);
  }
  ring.setBrightness(::brightness); // Restore configured brightness
}


void progressBarAnimation() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.setPixelColor(i, ring.Color(0, 255, 0)); // Green
    ring.show();
    delay(50); // Adjust progress speed
  }
}


void wifiSearchingAnimation() {
  static int pos = 0;
  ring.clear();
  ring.setPixelColor(pos, ring.Color(0, 0, 255)); // Blue
  pos = (pos + 1) % NUM_LEDS;
  ring.show();
  delay(100); // Adjust speed as needed
}


void wifiConnectingAnimation() {
  for (int i = 0; i < NUM_LEDS; i++) {
    int brightness = (sin(i * 0.2) + 1) * 127; // Sine wave effect
    ring.setPixelColor(i, ring.Color(0, brightness, brightness)); // Cyan
  }
  ring.show();
  delay(100); // Adjust wave speed
}



void wifiConnectedAnimation() {
  for (int i = 0; i < 3; i++) { // Flash three times
    ring.fill(ring.Color(0, 255, 0)); // Green
    ring.show();
    delay(200);
    ring.clear();
    ring.show();
    delay(200);
  }
}


void wifiFailedAnimation() {
  for (int i = 0; i < 3; i++) { // Flash three times
    ring.fill(ring.Color(255, 0, 0)); // Red
    ring.show();
    delay(200);
    ring.clear();
    ring.show();
    delay(200);
  }
}




