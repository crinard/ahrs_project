#include <stdio.h>
#include "gdl_90.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "WiFiServer.h"
#include <ESP8266WiFi.h>

// static WiFiUDP m_wifi_handle;

#ifndef APSSID
#define APSSID "Chris's iPhone"
#define APPSK  "hi there"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;
ESP8266WebServer m_server(80);

void handleRoot();

error_t GDL_90_init(void) {

    Serial.println("Configuring access point...");
    WiFi.softAP(ssid, password);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    m_server.on("/", handleRoot);
    m_server.begin();
    Serial.println("HTTP server started");

    return ERROR_OK;
};

error_t GDL_90_tx_cb(INS_Data_t *data) {
    m_server.handleClient();
    return ERROR_OK;
};

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  m_server.send(200, "text/html", "<h1>You are connected</h1>");
}