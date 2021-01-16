#pragma once
#include "../../src/API.hpp"
#include "OSCMessage.h"
#include <map>
#include <string>
using std::string;

// OSC Arg to std::string
std::string getOSCStringArg(OSCMessage &msg, int pos) {
  if (msg.isString(pos)) {
    int size = msg.getDataLength(pos);
    PRINT("allocating : ");
    PRINTLN(size);
    if (size > 0) {
      std::string buf(size - 1, '\0');
      if (!msg.getString(pos, &buf[0])) {
        PRINTLN("alloc failed");
      }
      PRINT("b : ");
      PRINTLN(buf.c_str());
      return buf;
    }
  }
  return {};
}
class OSCAPI {
public:
  OSCAPI(){

  };

  typedef enum { NONE = 0, SET = 1, GET = 2, CALL = 3 } MsgType;

  MsgType getType(OSCMessage &msg) {
    if (msg.isString(0)) {
      auto s = getOSCStringArg(msg, 0);
      if (s == "get") {
        return GET;
      } else if (s == "call") {
        return CALL;
      } else if (s == "set") {
        return SET;
      }
    }
    return NONE;
  }

  TypedArgList listFromOSCMessage(OSCMessage &msg, int start = 0) {
    TypedArgList res;
    for (int i = start; i < msg.size(); i++) {
      res.args.push_back(argFromOSCMessage(msg, i));
    }
    return res;
  }

  TypedArgBase::UPtr argFromOSCMessage(OSCMessage &msg, int o) {
    if (msg.isFloat(o)) {
      return std::make_unique<TypedArg<float>>(msg.getFloat(o));
    } else if (msg.isInt(o)) {
      return std::make_unique<TypedArg<int>>(msg.getInt(o));
    } else if (msg.isDouble(o)) {
      return std::make_unique<TypedArg<double>>(msg.getDouble(o));
    } else if (msg.isBoolean(o)) {
      return std::make_unique<TypedArg<bool>>(msg.getBoolean(o));
    } else if (msg.isString(o)) {
      return std::make_unique<TypedArg<std::string>>(getOSCStringArg(msg, o));
    }
    Serial.println("unsuported OSC member");
    return {};
  }

  bool listToOSCMessage(const TypedArgList &l, OSCMessage &msg, int start = 0);

  template <typename T>
  bool addToMsg(const TypedArgBase &a, OSCMessage &msg) const {
    if (a.is<T>()) {
      msg.add(a.get<T>());
      return true;
    }
    return false;
  }

  QResult processOSC(NodeBase *from, OSCMessage &msg, bool &needAnswer) {
    needAnswer = false;
    auto addr = getAddress(msg);
    if (addr == "/schema") {
      needAnswer = true;
      auto str = APISerializer::schemaFromNode(from);

      return QResult(new TypedArg<std::string>(str));
    }
    if (auto a = getEpFromMsg(from, msg)) {
      auto &api = *a.api;
      auto mt = getType(msg);
      Identifier localName = a.getLocalIdentifier(msg);
      if (localName.size() && localName[0] == '/') {
        localName = localName.substr(1);
      }
      Serial.print("msg type : ");
      Serial.print((int)mt);
      Serial.print("local name : ");
      Serial.println(localName.c_str());
      if (localName.size()) {
        // #if 1
        if (mt == GET) {
          needAnswer = true;
          PRINTLN("start get");
          return api.get(localName);
        }
#if 1
        if (mt == SET) {
          bool success = api.set(localName, {argFromOSCMessage(msg, 1)});
          return success ? QResult::ok()
                         : QResult::err("can't set " + localName);
        }
        if (mt == CALL) {
          needAnswer = true;
          return api.call(localName, listFromOSCMessage(msg, 1));
        } else {
          // defaults
          PRINT("gessing req from msg size : ");
          PRINTLN(msg.size());
          if (msg.size() == 1) { // set if argument
            if (api.canSet(localName)) {
              bool success = api.set(localName, {argFromOSCMessage(msg, 0)});
              return QResult(!success);
            } else if (api.canCall(localName)) {
              needAnswer = true;
              return api.call(localName, {argFromOSCMessage(msg, 0)});
            } else {
              return QResult::err("1 arg but can't set nor call");
            }
          } else if (msg.size() == 0) {
            if (api.canGet(localName)) { //  get if can get none
              needAnswer = true;
              return api.get(localName);
            } else if (api.canCall(localName)) {
              needAnswer = true;
              return api.call(localName, TypedArgList::empty());
            } else {
              return QResult::err("no arg but can't get nor call");
            }
          } else { // multi args
            if (api.canCall(localName)) {
              needAnswer = true;
              return api.call(localName, listFromOSCMessage(msg));
            } else {
              return QResult::err("multi args but can't call");
            }
          }
        }
#endif
      }

      return QResult::err(" can't reslove local name");
    }
    return QResult::err(" can't reslove api for namespace" + getAddress(msg));
  }

  struct OSCEndpoint {
    APIInstanceBase *api = nullptr;
    std::string childNameHint = "";
    OSCEndpoint(APIInstanceBase *_api = nullptr,
                const std::string &_childHint = "")
        : api(_api), childNameHint(_childHint) {}

    static char *getBuf() {
      static char strBuf[255];
      return strBuf;
    }

    MsgType type = SET;

    std::string getLocalIdentifier(OSCMessage &msg) {
      if (childNameHint.size()) {
        return childNameHint;
      }
      if (msg.isString(0)) {
        return getOSCStringArg(msg, 0);
      }
      return {};
    }

    operator bool() const { return api != nullptr; }
  };

  static std::string getAddress(OSCMessage &msg) {
    if (!msg.getAddress(OSCEndpoint::getBuf(), 0)) {
      OSCEndpoint::getBuf()[0] = '\0';
    }
    return std::string(OSCEndpoint::getBuf());
  }

protected:
  OSCEndpoint getEpFromMsg(NodeBase *from, OSCMessage &msg) {
    auto addr = getAddress(msg);
    auto addrV = StringHelpers::splitString(addr, '/');
    auto res = from->resolveAddr(addrV, 0);
    NodeBase *resolvedNode = res.first;
    int lastValidIdx = res.second;

    PRINT(" last ");
    PRINTLN(lastValidIdx);
    if (resolvedNode) {

      if (APIInstanceBase *resolvedAPI =
              getLinkedObj<APIInstanceBase>(resolvedNode)) {
        if (lastValidIdx < addrV.size() - 2) {
          PRINTLN("!!! OSC parse error");
        }
        auto cHint =
            (lastValidIdx == addrV.size() - 2 ? addrV[addrV.size() - 1] : "");
        return {resolvedAPI, cHint};
      } else {
        PRINT("!!! resolve can't be casted to API : ");
        PRINT(addrV.size());
        PRINT(" : ");
        PRINTLN(lastValidIdx);
      }
    } else {
      PRINT("!!! can't resolve :  ");
      PRINT(addrV.size());
      PRINT(" : ");
      PRINTLN(lastValidIdx);
    }
    return {};
  }
};

template <>
bool OSCAPI::addToMsg<std::string>(const TypedArgBase &a,
                                   OSCMessage &msg) const {
  if (a.is<std::string>()) {
    auto s = a.get<std::string>();
    msg.add(&s[0]);
    return true;
  }
  return false;
}

bool OSCAPI::listToOSCMessage(const TypedArgList &l, OSCMessage &msg,
                              int start) {
  bool success = l.size() - start > 0;
  if (!success) {
    Serial.println("nothing to add in resp msg");
  }
  for (int i = start; i < l.size(); i++) {
    auto &a = *l[i];
    bool added = addToMsg<bool>(a, msg) || addToMsg<int>(a, msg) ||
                 addToMsg<float>(a, msg) || addToMsg<double>(a, msg) ||
                 addToMsg<std::string>(a, msg);
    if (!added) {
      Serial.println("can't add to resp msg");
    }
    success &= added;
  }
  return success;
}
