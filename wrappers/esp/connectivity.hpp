#define USE_MDNS 1

// lightweight mdns
#define USE_NOMDNS 0

#if USE_MDNS
#include "ESPmDNS.h"
#endif

#include <OSCBundle.h>
#include <OSCMessage.h>
#include <WiFi.h>
#include <WiFiMulti.h>

// custom wifi settings
#include <esp_wifi.h>

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
    Serial.print("osc : ");                                                    \
    Serial.println(x);                                                         \
  }
#else
#define DBGOSC(x)                                                              \
  {}
#endif

#if DBG_WIFI
#define DBGWIFI(x)                                                             \
  {                                                                            \
    Serial.print("wifi : ");                                                   \
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

WiFiUDP udp, udpRcv;

// forward declare
bool sendPing();
void printEvent(WiFiEvent_t event);
void optimizeWiFi();
std::string getMac();

string instanceType;
std::string uid;
std::string instanceName;
unsigned long lastPingTime = 0;

bool connected = false;


// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  printEvent(event);
  switch (event) {
  case SYSTEM_EVENT_STA_GOT_IP:

    optimizeWiFi();
    // When connected set
    Serial.print("WiFi connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.getHostname());

#if USE_MDNS
    MDNS.begin((instanceName).c_str());
    MDNS.addService("rspstrio", "udp", conf::localPort);
    MDNS.addServiceTxt("rspstrio", "udp", "uuid",
                       (instanceType + "@" + getMac()).c_str());
#endif
    // initializes the UDP state
    // This initializes the transfer buffer
    udp.beginMulticast(conf::udpAddress, conf::announcePort);
    udpRcv.begin(conf::localPort);
    // digitalWrite(ledPin, HIGH);
    connected = true;
    lastPingTime = 0;
    sendPing();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    connected = false;
    break;
  default:
    // Serial.print("Wifi Event :");
    // Serial.println(event);
    break;
  }
}

void optimizeWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    PRINTLN("optimizing wifi connection");
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

    // we do not want to be sleeping !!!!
    if (!WiFi.setSleep(false)) {
      PRINTLN("can't stop sleep wifi");
    }
  } else {
    PRINTLN("can't optimize, not connected");
  }
}

void connectToWiFiTask(void *params) {
  WiFiMulti wifiMulti;
  if (strlen(net0.ssid)) {
    wifiMulti.addAP(net0.ssid, net0.pass);
  }
  for (auto &n : secrets::nets) {
    wifiMulti.addAP(n.ssid, n.pass);
  }
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      auto status = WiFi.status();
      unsigned long connectTimeout = 15000;
      if ((status == WL_CONNECT_FAILED) || (status == WL_CONNECTION_LOST) ||
          (status == WL_DISCONNECTED) || (status == WL_NO_SHIELD) ||
          (status == WL_IDLE_STATUS)) {
        DBGWIFI("force try reconnect ");
        DBGWIFI(status);
        wifiMulti.run(0);
        vTaskDelay(connectTimeout / portTICK_PERIOD_MS);
      }
      status = WiFi.status();

      if (status != WL_CONNECTED) {
        DBGWIFI("no connected");
        DBGWIFI(status);
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup(const string &type, const std::string &_uid) {
  instanceType = type;
  uid = _uid;
  delay(1000);
  DBGWIFI("setting up : ");
  DBGWIFI(uid.c_str());
  Serial.println("\n");

  delay(100);
  instanceName = instanceType;
  if (uid.size())
    instanceName += "_" + uid;

  WiFi.setHostname(instanceName.c_str());
  // register event handler
  WiFi.onEvent(WiFiEvent);

  // here we could setSTA+AP if needed (supported by wifiMulti normally)
  xTaskCreatePinnedToCore(connectToWiFiTask, "keep wifi", 5000, NULL, 1, NULL,
                          CONFIG_ARDUINO_RUNNING_CORE);
}

bool handleConnection() {

  if (connected)
    sendPing();
  return connected;
}

String joinToString(vector<float> p) {
  String res;
  for (auto &e : p) {
    res += ", " + String(e);
  }
  return res;
}

bool receiveOSC(OSCBundle &bundle) {
  if (!connected)
    return false;
  int size = udpRcv.parsePacket();

  if (size > 0) {
    // Serial.print("size : ");
    // Serial.println(size);
    while (size--) {
      bundle.fill(udpRcv.read());
    }

    if (bundle.size() > 0) {
      // Serial.print("bundle received : ");
      // Serial.println(bundle.size());
      return true;
    } else if (bundle.hasError()) {
      Serial.print("bundle err : ");
      Serial.println(bundle.getError());
    }
  }
  return false;
}

void sendOSCResp(OSCMessage &m) {
  udpRcv.beginPacket(udpRcv.remoteIP(), udpRcv.remotePort());
  m.send(udpRcv);
  udpRcv.endPacket();
  DBGOSC("sent resp");
  DBGOSC(udpRcv.remoteIP().toString().c_str());
  DBGOSC(std::to_string(udpRcv.remotePort()).c_str());
}

void sendOSC(const char *addr, const vector<float> &a) {

  if (!connected) {
    return;
  }
  OSCMessage msg(addr);
  msg.add(instanceName.c_str());
  for (int i = 0; i < a.size(); i++) {
    msg.add(a[i]);
  }

  udp.beginMulticastPacket();
  msg.send(udp);
  udp.endPacket();
  // DBGOSC("sending" + String(addr) + " " + joinToString(a));
}

void sendOSC(const char *addr, int id, int val) {
  if (!connected) {
    return;
  }
  {
    OSCMessage msg(addr);
    msg.add(instanceName.c_str());
    msg.add((int)id);
    msg.add((int)val);

    udp.beginMulticastPacket();
    msg.send(udp);
    udp.endPacket();
  }

  // if (std::string(addr) != "/ping")
  DBGOSC("sendingv : " + String(addr) + " " + String(id) + " : " + String(val));
}

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

void sendOSC(const char *addr, int val) {
  if (!connected) {
    return;
  }
  OSCMessage msg(addr);
  msg.add(instanceName.c_str());
  msg.add((int)val);

  udp.beginMulticastPacket();
  msg.send(udp);
  udp.endPacket();
  DBGOSC("sending" + String(addr) + " : " + String(val));
}

bool sendPing() {
  if (!connected) {
    return false;
  }
  auto time = millis();
  int pingTime = 3000;
  if ((time - lastPingTime) > (unsigned long)pingTime) {
    sendOSC("/ping", pingTime, conf::localPort);
    sendNoMDNSAnnounce();
    lastPingTime = time;
    return true;
  }
  return false;
}

std::string getMac() {
  uint8_t mac_id[6];
  esp_efuse_mac_get_default(mac_id);
  char esp_mac[13];
  sprintf(esp_mac, "%02x%02x%02x%02x%02x%02x", mac_id[0], mac_id[1], mac_id[2],
          mac_id[3], mac_id[4], mac_id[5]);
  return std::string(esp_mac);
}

void printEvent(WiFiEvent_t event) {

  // DBGWIFI("[WiFi-event] event: %d : ", event);

  switch (event) {
  case SYSTEM_EVENT_WIFI_READY:
    DBGWIFI("WiFi interface ready");
    break;
  case SYSTEM_EVENT_SCAN_DONE:
    DBGWIFI("Completed scan for access points");
    break;
  case SYSTEM_EVENT_STA_START:
    DBGWIFI("WiFi client started");
    break;
  case SYSTEM_EVENT_STA_STOP:
    DBGWIFI("WiFi clients stopped");
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    DBGWIFI("Connected to access point");
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    DBGWIFI("Disconnected from WiFi access point");
    break;
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    DBGWIFI("Authentication mode of access point has changed");
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    Serial.print("Obtained IP address: ");
    DBGWIFI(WiFi.localIP());
    break;
  case SYSTEM_EVENT_STA_LOST_IP:
    DBGWIFI("Lost IP address and IP address is reset to 0");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    DBGWIFI("WiFi Protected Setup (WPS): succeeded in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    DBGWIFI("WiFi Protected Setup (WPS): failed in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
    DBGWIFI("WiFi Protected Setup (WPS): timeout in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_PIN:
    DBGWIFI("WiFi Protected Setup (WPS): pin code in enrollee mode");
    break;
  case SYSTEM_EVENT_AP_START:
    DBGWIFI("WiFi access point started");
    break;
  case SYSTEM_EVENT_AP_STOP:
    DBGWIFI("WiFi access point  stopped");
    break;
  case SYSTEM_EVENT_AP_STACONNECTED:
    DBGWIFI("Client connected");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    DBGWIFI("Client disconnected");
    break;
  case SYSTEM_EVENT_AP_STAIPASSIGNED:
    DBGWIFI("Assigned IP address to client");
    break;
  case SYSTEM_EVENT_AP_PROBEREQRECVED:
    DBGWIFI("Received probe request");
    break;
  case SYSTEM_EVENT_GOT_IP6:
    DBGWIFI("IPv6 is preferred");
    break;
  case SYSTEM_EVENT_ETH_START:
    DBGWIFI("Ethernet started");
    break;
  case SYSTEM_EVENT_ETH_STOP:
    DBGWIFI("Ethernet stopped");
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    DBGWIFI("Ethernet connected");
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    DBGWIFI("Ethernet disconnected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    DBGWIFI("Obtained IP address");
    break;
  default:
    DBGWIFI("unknown");
    break;
  }
}
} // namespace connectivity
