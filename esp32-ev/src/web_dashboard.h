/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  ESP32-WROVER-E DevKit (DevKitC-VE) - built in WiFi.

  Description:
  WiFi web dashboard. The ESP32 makes its own WiFi network (access point),
  serves the dashboard page (web/index.html, compiled into the firmware) and
  pushes every transmit array line to the page over a WebSocket.
  Read only - the page cannot control the car.

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  Join WiFi AP_SSID (config.h) and open http://192.168.4.1
*/
#ifndef EV_WEB_DASHBOARD_H            // include guard
#define EV_WEB_DASHBOARD_H            // mark as included

void webBegin();                      // start WiFi + web server (call in setup)
void webPush(const char* line);       // send one "$EV,..." line to every open page
void webLoop();                       // housekeeping (call every loop)

#endif                                // EV_WEB_DASHBOARD_H
