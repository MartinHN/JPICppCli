#pragma once

#define USE_MDNS 1

// lightweight mdns
#define USE_NOMDNS 0

#if USE_MDNS
#include "ESPmDNS.h"
#endif

#include <OSCBundle.h>
#include <OSCMessage.h>
#include <WiFi.h>
#include "AsyncWiFiMulti.h"

// custom wifi settings
#include <esp_wifi.h>

#include "driver/adc.h" // for deactivating adc

using std::string;
using std::vector;

struct net {
  const char *ssid;
  const char *pass;
};
// if compile error here : you need to add your secrets as vector
#include "secrets.hpp"
net net0 = {"mange ma chatte", "sucemonbeat"};
// adds pass from secret networks...

#if DBG_OSC
#define DBGOSC(x)                                                              \
  {                                                                            \
    Serial.print("[osc] ");                                                    \
    Serial.println(x);                                                         \
  }
#else
#define DBGOSC(x)                                                              \
  {}
#endif

#if DBG_WIFI
#define DBGWIFI(x)                                                             \
  {                                                                            \
    Serial.print("[wifi] ");                                                   \
    Serial.println(x);                                                         \
  }
#else
#define DBGWIFI(x)                                                             \
  {}
#endif
namespace connectivity {

namespace conf {
const IPAddress udpAddress(230, 1, 1, 1);
const int announcePort = 4000;
const int localPort = 3000;
}; // namespace conf

WiFiUDP udpRcv;

// forward declare
bool sendPing();
void printEvent(WiFiEvent_t event);
void optimizeWiFi();
std::string getMac();

string instanceType;
std::string uid;
std::string instanceName;
unsigned long lastPingTime = 0;

volatile bool connected = false;
volatile bool isWifiDisabled = false;
volatile bool hasBeenDeconnected = false;

std::string mdnsSrvTxt;
AsyncWiFiMulti wifiMulti;
// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  printEvent(event);
  switch (event) {
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_GOT_IP:

    optimizeWiFi();

    // When connected set
    Serial.print("WiFi connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.getHostname());

#if USE_MDNS
    if (!MDNS.begin(instanceName.c_str()))
      Serial.print("!!!fail to start mdns service");
    else
      Serial.print("start mdns service ");
    Serial.println(instanceName.c_str());
    MDNS.addService("rspstrio", "udp", conf::localPort);
    MDNS.addServiceTxt("rspstrio", "udp", "uuid", mdnsSrvTxt.c_str());
#endif
    // initializes the UDP state
    // This initializes the transfer buffer
    // udp.beginMulticast(conf::udpAddress, conf::announcePort);
    udpRcv.begin(conf::localPort);
    // digitalWrite(ledPin, HIGH);
    connected = true;
    lastPingTime = 0;
    // sendPing();
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#if USE_MDNS
    MDNS.end();
#endif
    Serial.println("WiFi lost connection");
    hasBeenDeconnected = true;
    connected = false;
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_SCAN_DONE:
    PRINTLN(">>>scan cb");
    wifiMulti.handleScanDone(5000);

    break;
  default:
    // Serial.print("Wifi Event :");
    // Serial.println(event);
    break;
  }
}

void reduceWifi() {
  PRINTLN("reducing wifi connection");
  if (!WiFi.setSleep(true)) {
    PRINTLN("can't stop sleep wifi");
  }
  // yes power saving here
  // Mapping Table {Power, max_tx_power} = {{8,   2}, {20,  5}, {28,  7}, {34,  8}, {44, 11},
  // *                                                      {52, 13}, {56, 14}, {60, 15}, {66, 16}, {72, 18}, {80, 20}}.
  if (auto err = esp_wifi_set_max_tx_power(28)) {
    PRINT("can't decrease power : ");
    switch (err) {
      //: WiFi is not initialized by esp_wifi_init
    case (ESP_ERR_WIFI_NOT_INIT):
      PRINTLN("wifi not inited");
      //: WiFi is not started by esp_wifi_start
    case (ESP_ERR_WIFI_NOT_STARTED):
      PRINTLN("wifi not started");
      //: invalid argument, e.g. parameter is out of range
    // case (ESP_ERR_WIFI_ARG):
    //   PRINTLN("wrong args");
    default:
      PRINTLN("unknown err");
    }
  }
}
void optimizeWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    // reduceWifi();
    // return;
    PRINTLN("optimizing wifi connection");
    // we do not want to be sleeping !!!!
    if (!WiFi.setSleep(false)) {
      PRINTLN("can't stop sleep wifi");
    }
    // no power saving here
    if (auto err = esp_wifi_set_max_tx_power(82)) {
      PRINT("can't increase power : ");
      switch (err) {
        //: WiFi is not initialized by esp_wifi_init
      case (ESP_ERR_WIFI_NOT_INIT):
        PRINTLN("wifi not inited");
        //: WiFi is not started by esp_wifi_start
      case (ESP_ERR_WIFI_NOT_STARTED):
        PRINTLN("wifi not started");
        //: invalid argument, e.g. parameter is out of range
      // case (ESP_ERR_WIFI_ARG):
      //   PRINTLN("wrong args");
      default:
        PRINTLN("unknown err");
      }
    }

  } else {
    PRINTLN("can't optimize, not connected");
  }
}

void setWifiDisabled(bool b) {
  isWifiDisabled = b;
  wifiMulti.hasHandledResultAndClear();
}

void connectToWiFiTask(void *params) {
  if (strlen(net0.ssid)) {
    wifiMulti.addAP(net0.ssid, net0.pass);
  }
  for (auto &n : secrets::nets) {
    wifiMulti.addAP(n.ssid, n.pass);
  }
  int scanIntervalMs = 20111;
  int scanResultWaitMs = 511;
  for (;;) {
    Serial.printf("Wifi() - Free Stack Space: %d\n", uxTaskGetStackHighWaterMark(NULL));
    Serial.flush();
    if (isWifiDisabled) {
      WiFi.enableSTA(false);
      adc_power_off();
    } else if (!connected) {
      // WiFi.status() != WL_CONNECTED may incur weird side effect?
      // if (connected) {
      //   connected = false;
      //   DBGWIFI("manually set  disconnected flag");
      // }

      if (wifiMulti.hasHandledResultAndClear()) {
        DBGWIFI("has scanned once, wait before next scan");
        WiFi.enableSTA(false);
        adc_power_off();
        vTaskDelay(scanIntervalMs / portTICK_PERIOD_MS);
        continue;
      }
      adc_power_on();
      WiFi.enableSTA(true);

      auto scanStatus = WiFi.scanComplete();
      if (scanStatus == WIFI_SCAN_RUNNING) {
        DBGWIFI("waiting for scan to end");
        vTaskDelay(scanResultWaitMs / portTICK_PERIOD_MS);
        continue;
      }

      auto status = wifiMulti.run(0);
      if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
        DBGWIFI("waiting for scan to end post");
        vTaskDelay(scanResultWaitMs / portTICK_PERIOD_MS);
        continue;
      }

      if ((status == WL_CONNECT_FAILED) || (status == WL_CONNECTION_LOST) || (status == WL_DISCONNECTED) || (status == WL_NO_SHIELD) ||
          (status == WL_IDLE_STATUS)) {
        DBGWIFI("will try reconnect ");
        DBGWIFI(status);
      }
    }
    vTaskDelay(scanIntervalMs / portTICK_PERIOD_MS);
    DBGWIFI("back to wifi");
  }
}

inline bool ends_with(std::string const &value, std::string const &ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void setup(const string &type, const std::string &_uid) {
  instanceType = type;
  uid = _uid;
  // delay(1000);
  DBGWIFI("setting up : ");
  DBGWIFI(uid.c_str());
  Serial.println("\n");
  mdnsSrvTxt = instanceType + "@" + getMac();
  // delay(100);
  instanceName = instanceType;
  if (uid.size()) {

    if ((uid.rfind(instanceName + "_", 0) == 0) &&
        uid.size() > instanceName.size() + 1) {
      instanceName = uid;
    } else {
      instanceName += "_" + uid;
    }
    string localSuf = ".local";
    if (ends_with(instanceName, localSuf)) {
      instanceName =
          instanceName.substr(0, instanceName.length() - localSuf.length());
    }
  }

  WiFi.setHostname(instanceName.c_str());
  // register event handler
  WiFi.onEvent(WiFiEvent);

  // here we could setSTA+AP if needed (supported by wifiMulti normally)
  int stackSz = 4048; // 5000;
  xTaskCreatePinnedToCore(connectToWiFiTask, "keepwifi", stackSz, NULL, 5, NULL, (ARDUINO_RUNNING_CORE + 1) % 2);
}

bool handleConnection() {
#if USE_NOMDNS
  if (connected)
    sendPing();
#endif
  return connected;
}

String joinToString(vector<float> p) {
  String res;
  for (auto &e : p) {
    res += ", " + String(e);
  }
  return res;
}

bool receiveOSC(OSCMessage &bundle) {
  if (!connected)
    return false;

  // while (udpRcv.available()) {

  int size = udpRcv.parsePacket();
  if (size == 0)
    return false;
  while (size > 0) {
    // Serial.print("size : ");
    // Serial.println(size);
    while (size--) {
      bundle.fill(udpRcv.read());
    }
    // if (bundle.hasError() || ) {
    // size = udpRcv.parsePacket();

    //   while (size--) {
    //     bundle.fill(udpRcv.read());
    //   }
  }
  if (udpRcv.available()) {
    Serial.print("more udp data available");
  }

  if (bundle.hasError()) {
    Serial.print("bundle err : ");
    Serial.println(bundle.getError());
    return false;
  }
  // }
  return true;
}

void sendOSCResp(OSCMessage &m) {
  udpRcv.beginPacket(udpRcv.remoteIP(), udpRcv.remotePort());
  m.send(udpRcv);
  udpRcv.endPacket();
  udpRcv.flush();

  // DBGOSC((String("sent resp : ") + m.getAddress() + "(" + String(m.size()) +
  //         " args) , ".remoteIP().toString() + " " +
  //         String(udpRcv.remotePort()))
  //            .c_str());
}

// void sendOSC(const char *addr, const vector<float> &a) {

//   if (!connected) {
//     return;
//   }
//   OSCMessage msg(addr);
//   msg.add(instanceName.c_str());
//   for (int i = 0; i < a.size(); i++) {
//     msg.add(a[i]);
//   }

//   udp.beginMulticastPacket();
//   msg.send(udp);
//   udp.endPacket();
//   // DBGOSC("sending" + String(addr) + " " + joinToString(a));
// }

// void sendOSC(const char *addr, int id, int val) {
//   if (!connected) {
//     return;
//   }
//   {
//     OSCMessage msg(addr);
//     msg.add(instanceName.c_str());
//     msg.add((int)id);
//     msg.add((int)val);

//     udp.beginMulticastPacket();
//     msg.send(udp);
//     udp.endPacket();
//   }

//   // if (std::string(addr) != "/ping")
//   DBGOSC("sendingv : " + String(addr) + " " + String(id) + " : " +
//   String(val));
// }

void sendNoMDNSAnnounce() {
#if USE_NOMDNS
  if (!connected) {
    return;
  }
  std::string announce = R"({"type":")" + instanceType + R"(","id":")" + uid +
                         R"(","port":)" + std::to_string(conf::localPort) + "}";
  OSCMessage msg("/announce");
  msg.add(announce.c_str());
  udp.beginMulticastPacket();
  msg.send(udp);
  udp.endPacket();
  DBGOSC("sending announce");
  DBGOSC(announce.c_str());
#endif
}

// void sendOSC(const char *addr, int val) {
//   if (!connected) {
//     return;
//   }
//   OSCMessage msg(addr);
//   msg.add(instanceName.c_str());
//   msg.add((int)val);

//   udp.beginMulticastPacket();
//   msg.send(udp);
//   udp.endPacket();
//   DBGOSC("sending" + String(addr) + " : " + String(val));
// }

// bool sendPing() {
//   if (!connected) {
//     return false;
//   }
//   auto time = millis();
//   int pingTime = 3000;
//   if ((time - lastPingTime) > (unsigned long)pingTime) {
//     sendOSC("/ping", pingTime, conf::localPort);
//     sendNoMDNSAnnounce();
//     lastPingTime = time;
//     return true;
//   }
//   return false;
// }

std::string getMac() {
  uint8_t mac_id[6];
  esp_efuse_mac_get_default(mac_id);
  char esp_mac[13];
  sprintf(esp_mac, "%02x%02x%02x%02x%02x%02x", mac_id[0], mac_id[1], mac_id[2],
          mac_id[3], mac_id[4], mac_id[5]);
  return std::string(esp_mac);
}

void printEvent(WiFiEvent_t event) {
  switch (event) {
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_READY:
    DBGWIFI("WiFi interface ready");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_SCAN_DONE:
    DBGWIFI("Completed scan for access points");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_START:
    DBGWIFI("WiFi client started");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_STOP:
    DBGWIFI("WiFi clients stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_CONNECTED:
    DBGWIFI("Connected to access point");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    DBGWIFI("Disconnected from WiFi access point");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    DBGWIFI("Authentication mode of access point has changed");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_GOT_IP:
    DBGWIFI("Obtained IP address: " + String(WiFi.localIP()));
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_LOST_IP:
    DBGWIFI("Lost IP address and IP address is reset to 0");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_SUCCESS:
    DBGWIFI("WiFi Protected Setup (WPS): succeeded in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_FAILED:
    DBGWIFI("WiFi Protected Setup (WPS): failed in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_TIMEOUT:
    DBGWIFI("WiFi Protected Setup (WPS): timeout in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_PIN:
    DBGWIFI("WiFi Protected Setup (WPS): pin code in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_START:
    DBGWIFI("WiFi access point started");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STOP:
    DBGWIFI("WiFi access point  stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    DBGWIFI("Client connected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    DBGWIFI("Client disconnected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    DBGWIFI("Assigned IP address to client");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    DBGWIFI("Received probe request");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    DBGWIFI("IPv6 is preferred");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_START:
    DBGWIFI("Ethernet started");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_STOP:
    DBGWIFI("Ethernet stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_CONNECTED:
    DBGWIFI("Ethernet connected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_DISCONNECTED:
    DBGWIFI("Ethernet disconnected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_GOT_IP:
    DBGWIFI("Obtained IP address");
    break;
  default:
    DBGWIFI("unknown wifi event " + String(event));
    break;
  }
}
} // namespace connectivity
