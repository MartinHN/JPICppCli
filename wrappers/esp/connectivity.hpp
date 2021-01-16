#include "ESPmDNS.h"
#include <OSCBundle.h>
#include <OSCMessage.h>
#include <WiFi.h>
#include <WiFiMulti.h>

using std::string;
using std::vector;

struct net {
  const char *ssid;
  const char *pass;
};
#include "secrets.hpp"
net net0 = secrets::nets[0]; //.ne {"tinmarphone", "tinmarphone"};
// adds pass from secret networks...

#define MULTI 1

#define DBGOSC(x)                                                              \
  Serial.print("osc : ");                                                      \
  Serial.println(x);

#define DBGWIFI(x)                                                             \
  Serial.print("wifi : ");                                                     \
  Serial.println(x);

namespace connectivity {

namespace conf {
const IPAddress udpAddress(230, 1, 1, 1);
const int announcePort = 4000;
const int localPort = 3000;
}; // namespace conf

WiFiUDP udp, udpRcv;
#if MULTI
WiFiMulti wifiMulti;
#endif

// forward declare
bool sendPing();

// values
const char *ssid = net0.ssid;
const char *password = net0.pass;

string mdnsPrefix;
std::string uid;
unsigned long lastPingTime = 0;

bool connected = false;

void printEvent(WiFiEvent_t event);

// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  printEvent(event);
  switch (event) {
  case SYSTEM_EVENT_STA_GOT_IP:
    // When connected set
    Serial.print("WiFi connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.getHostname());
    // initializes the UDP state
    // This initializes the transfer buffer
    udp.beginMulticast(conf::udpAddress, conf::announcePort);
    udpRcv.begin(conf::localPort);
    // digitalWrite(ledPin, HIGH);
    connected = true;
    sendPing();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    // Serial.println("WiFi lost connection");
    connected = false;
    break;
  default:
    // Serial.print("Wifi Event :");
    // Serial.println(event);
    break;
  }
}
void connectToWiFi() {
#if MULTI
  DBGWIFI("try connect");
  if (wifiMulti.run(6000) != WL_CONNECTED) {
    DBGWIFI("no connected");
  };
#else

  Serial.print("disconnect first ...");
  // delete old config
  WiFi.disconnect(true);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(("sensor" + uid).c_str());
  delay(500);
  Serial.print("Tentative de connexion...");
  WiFi.begin(ssid, password);
#endif
}

void setup(const string &type, const std::string &_uid) {
  mdnsPrefix = type;
  uid = _uid;
  delay(1000);
  Serial.print("setting up : ");
  Serial.println(uid.c_str());
  Serial.println("\n");

  delay(100);
  std::string mdnsName = mdnsPrefix + uid;
  MDNS.begin(mdnsName.c_str());
  WiFi.setHostname(mdnsName.c_str());
  // register event handler
  WiFi.onEvent(WiFiEvent);
#if MULTI
  wifiMulti.addAP(net0.ssid, net0.pass);
  for (auto &n : secrets::nets) {
    wifiMulti.addAP(n.ssid, n.pass);
  }
#else
  WiFi.mode(WIFI_STA);
#endif
  connectToWiFi();
}

bool handleConnection() {
#if MULTI
  auto status = wifiMulti.run();
  if (status == WL_NO_SSID_AVAIL) {
    // skip while scan running
    delay(100);
    DBGWIFI("scan running");
    return false;
  }
  if (status != WL_CONNECTED) {
    DBGWIFI("reconnecting");
    delay(3000);
    connectToWiFi();
    delay(3000);
  } else {
    connected = true;
  }
#else
  if (!connected) {
    // for (int i = 0 ; i < 1; i++) {
    // 	delay(2000);
    // 	digitalWrite(ledPin, !digitalRead(ledPin));
    // }

    connectToWiFi();

    int maxAttempt = 100;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
      maxAttempt--;
      if (maxAttempt < 0) {
        break;
      }
    }
  }
#endif
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
    Serial.print("size : ");
    Serial.println(size);
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
  msg.add(uid.c_str());
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
  OSCMessage msg(addr);
  msg.add(uid.c_str());
  msg.add((int)id);
  msg.add((int)val);

  udp.beginMulticastPacket();
  msg.send(udp);
  udp.endPacket();
  DBGOSC("sending" + String(addr) + " " + String(id) + " : " + String(val));
}

void sendAnnounce() {
  if (!connected) {
    return;
  }
  std::string announce = R"({"type":")" + mdnsPrefix + R"(","id":")" + uid +
                         R"(","port":)" + std::to_string(conf::localPort) + "}";
  OSCMessage msg("/announce");
  msg.add(announce.c_str());
  udp.beginMulticastPacket();
  msg.send(udp);
  udp.endPacket();
  DBGOSC("sending announce");
  DBGOSC(announce.c_str());
}

void sendOSC(const char *addr, int val) {
  if (!connected) {
    return;
  }
  OSCMessage msg(addr);
  msg.add(uid.c_str());
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
  if ((time - lastPingTime) > pingTime) {
    sendOSC("/ping", pingTime, conf::localPort);
    sendAnnounce();
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

  Serial.printf("[WiFi-event] event: %d : ", event);

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
    break;
  }
}
} // namespace connectivity
