#include <Arduino.h>
#include <SPI.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#include <IPAddress.h>
#include <ESPmDNS.h>
#endif

#include "PluginManager.h"

#include "plugins/DrawPlugin.h"
#include "plugins/BreakoutPlugin.h"
#include "plugins/SnakePlugin.h"
#include "plugins/GameOfLifePlugin.h"
#include "plugins/StarsPlugin.h"
#include "plugins/LinesPlugin.h"
#include "plugins/CirclePlugin.h"
#include "plugins/RainPlugin.h"
#include "plugins/FireworkPlugin.h"
#include "plugins/PongClockPlugin.h"

#ifdef ENABLE_SERVER
#include "plugins/BigClockPlugin.h"
#include "plugins/ClockPlugin.h"
#include "plugins/WeatherPlugin.h"
#include "plugins/AnimationPlugin.h"
#endif

#include "websocket.h"
#include "secrets.h"
#include "ota.h"
#include "webserver.h"
#include "screen.h"

unsigned long previousMillis = 0;
unsigned long interval = 30000;

PluginManager pluginManager;
SYSTEM_STATUS currentStatus = NONE;

unsigned long lastConnectionAttempt = 0;
const unsigned long connectionInterval = 10000;

void connectToWiFi()
{
  Serial.println("Connecting to Wi-Fi...");

  // Delete old config
  WiFi.disconnect(true);

#if defined(IP_ADDRESS) && defined(GWY) && defined(SUBNET) && defined(DNS1) && defined(DNS2)
  auto ip = IPAddress();
  ip.fromString(IP_ADDRESS);

  auto gwy = IPAddress();
  gwy.fromString(GWY);

  auto subnet = IPAddress();
  subnet.fromString(SUBNET);

  auto dns1 = IPAddress();
  dns1.fromString(DNS1);

  auto dns2 = IPAddress();
  dns2.fromString(DNS2);

  WiFi.config(ip, gwy, subnet, dns1, dns2);
#endif

  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  // Check connection result
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

#ifdef ESP32
    if(MDNS.begin(WIFI_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
      MDNS.setInstanceName(WIFI_HOSTNAME);
    } else {
      Serial.println("Could not start mDNS!");
    }
#endif
  }
  else
  {
    Serial.println("\nFailed to connect to Wi-Fi. Please check credentials.");
  }

  lastConnectionAttempt = millis();
}

void setup()
{
  Serial.begin(115200);

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

// server
#ifdef ENABLE_SERVER
  connectToWiFi();

  // set time server
  configTzTime(TZ_INFO, NTP_SERVER);

  initOTA(server);
  initWebsocketServer(server);
  initWebServer();
#endif

  Screen.setup();

  pluginManager.addPlugin(new DrawPlugin());
  pluginManager.addPlugin(new BreakoutPlugin());
  pluginManager.addPlugin(new SnakePlugin());
  pluginManager.addPlugin(new GameOfLifePlugin());
  pluginManager.addPlugin(new StarsPlugin());
  pluginManager.addPlugin(new LinesPlugin());
  pluginManager.addPlugin(new CirclePlugin());
  pluginManager.addPlugin(new RainPlugin());
  pluginManager.addPlugin(new FireworkPlugin());
  pluginManager.addPlugin(new PongClockPlugin());

#ifdef ENABLE_SERVER
  pluginManager.addPlugin(new BigClockPlugin());
  pluginManager.addPlugin(new ClockPlugin());
  pluginManager.addPlugin(new WeatherPlugin());
  pluginManager.addPlugin(new AnimationPlugin());
#endif

  pluginManager.init();
}

void loop()
{
  pluginManager.runActivePlugin();

  if (WiFi.status() != WL_CONNECTED && millis() - lastConnectionAttempt > connectionInterval)
  {
    Serial.println("Lost connection to Wi-Fi. Reconnecting...");
    connectToWiFi();
  }

#ifdef ENABLE_SERVER
  cleanUpClients();
#endif
  delay(1);
}