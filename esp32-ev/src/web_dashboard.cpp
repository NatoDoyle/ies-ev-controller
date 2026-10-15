/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  ESP32-WROVER-E DevKit (DevKitC-VE) - built in WiFi.

  Description:
  Implementation of the WiFi web dashboard (see web_dashboard.h).
    /      -> the dashboard page
    /ws    -> WebSocket, one "$EV,..." text message per transmit (10 Hz)
    /data  -> the latest line as plain text (handy for testing in a browser)

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  SSID / password in include/config.h.
*/
#include <Arduino.h>                  // Serial, millis
#include <WiFi.h>                     // ESP32 WiFi
#include <ESPAsyncWebServer.h>        // web server + WebSocket (runs in the background)
#include "config.h"                   // AP_SSID, AP_PASS
#include "web_dashboard.h"            // our declarations

// web/index.html is linked into the firmware by "board_build.embed_txtfiles"
// in platformio.ini. The linker makes these two symbols mark its start/end.
extern const uint8_t index_html_start[] asm("_binary_web_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_web_index_html_end");

static AsyncWebServer server(80);     // HTTP server on port 80
static AsyncWebSocket ws("/ws");      // WebSocket at ws://192.168.4.1/ws
static String lastLine = "";          // newest transmit array line (for /data)
static bool started = false;          // true once WiFi is up
static uint32_t lastCleanup = 0;      // time of the last client clean up

void webBegin() {
  WiFi.mode(WIFI_AP);                 // access point only (eduroam needs a login, so we make our own network)
  if (!WiFi.softAP(AP_SSID, AP_PASS)) {  // start the network
    Serial.println("# WiFi AP failed - serial dashboard still works");  // '#' = not data
    return;                           // carry on without the web dashboard
  }
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // lower power = fewer brown-outs, still fine for a room
  server.addHandler(&ws);             // attach the WebSocket to the server

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {  // dashboard page
    size_t len = index_html_end - index_html_start - 1;          // -1 drops the added '\0'
    request->send(200, "text/html", index_html_start, len);      // send the page
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) {  // latest line
    request->send(200, "text/plain", lastLine);                      // plain text
  });

  server.onNotFound([](AsyncWebServerRequest* request) {  // anything else
    request->redirect("/");           // go to the dashboard (helps phone captive checks)
  });

  server.begin();                     // start serving
  started = true;                     // web dashboard is running
  Serial.print("# Web dashboard: join WiFi \"");  // tell the user how to connect
  Serial.print(AP_SSID);              // network name
  Serial.print("\" and open http://");  // then the address
  Serial.println(WiFi.softAPIP());    // usually 192.168.4.1
}

void webPush(const char* line) {
  if (!started) return;               // WiFi not running
  lastLine = line;                    // keep it for /data
  lastLine.trim();                    // remove the '\n'
  if (ws.count() > 0) {               // someone is watching
    ws.textAll(lastLine);             // push to every page (a stuck page just misses lines, others still update)
  }
}

void webLoop() {
  if (!started) return;               // WiFi not running
  if (millis() - lastCleanup >= 1000) {  // once a second
    lastCleanup = millis();           // remember when
    ws.cleanupClients();              // drop pages that have closed
  }
}
