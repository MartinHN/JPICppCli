#pragma once

#if USE_SERIALIZER

// #if USE_CEREAL
// #include "../lib/cereal/include/cereal/archives/json.hpp"
// #endif // USE_CEREAL

#include "APIInstance.h"
#include <string>

namespace APISerializer {

#if USE_CEREAL

struct SerializableAPI {
  SerializableAPI(APIInstanceBase *api) : apiI(api) {}
  template <class AR> void serialize(AR &ar) const {
    const auto &members = apiI->getAPI()->members;
    for (const auto &mm : members) {

      bool success = false;
      auto *m = apiI->getMemberInstance(mm.first);
      if (m) {
        success = checkAdd<float>(m, ar, mm.first) ||
                  checkAdd<int>(m, ar, mm.first) ||
                  checkAdd<double>(m, ar, mm.first) ||
                  checkAdd<std::string>(m, ar, mm.first) ||
                  checkAdd<bool>(m, ar, mm.first);
      }
      if (!success) {
        PRINTLN("!!! serialization error");
      }
    }
  }
  template <typename T, typename AR>
  bool checkAdd(MemberInstanceBase *mb, AR &ar,
                const std::string &mname) const {
    if (auto m = dynamic_cast<MemberInstanceT<T> *>(mb)) {
      // const std::string &mname = m.first;
      // if (mname == "test") {
      // const auto &ref = m.second->get<T>(apiI);
      // PRINT("before ");
      // PRINT(ref);
      // PRINT(" ");
      // PRINTLN((int)(void *)(&ref));
      // }
      ar(cereal::make_nvp(mname, m->getRef()));
      // if (mname == "test") {
      // const auto &ref = m.second->getRef<T>();
      // PRINT("after ");
      // PRINT(ref);
      // PRINT(" ");
      // PRINTLN((int)(void *)(&ref));
      // }
      return true;
    }
    return false;
  }
  APIInstanceBase *apiI;
};

struct NodeStateAPI {
  NodeStateAPI(NodeBase *n, std::vector<NodeBase *> &usedNodes)
      : node(n), usedNodes(usedNodes) {
    usedNodes.push_back(n);
  }
  template <class AR> void serialize(AR &ar) const {
    if (auto nm = dynamic_cast<MapNode *>(node)) {
      for (auto &m : nm->m) {
        PRINT("used");
        PRINTLN(usedNodes.size());
        if (std::find(usedNodes.begin(), usedNodes.end(), m.second) !=
            std::end(usedNodes)) {

          PRINT("cyclic dep : ");
          PRINTLN(m.first.c_str());
          continue;
        }
        PRINT("adding node ");
        PRINTLN(m.first.c_str());
        ar(cereal::make_nvp(m.first, NodeStateAPI(m.second, usedNodes)));
      }
    }
    if (auto api = dynamic_cast<APIInstanceBase *>(node)) {
      // ar(cereal::make_nvp("props", SerializableAPI(*api)));
      SerializableAPI(api).serialize(ar);
    }
  }

  NodeBase *node;
  std::vector<NodeBase *> &usedNodes;
};
#endif // USE_CEREAL

static std::string stateFromNode(NodeBase *n) {
  std::string res;
#if USE_CEREAL
  std::ostringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    std::vector<NodeBase *> usedNodes;

    NodeStateAPI(n, usedNodes).serialize(archive);

    // need delete archive to flush
  }
  res = stream.str();
#else
#error "not supported"
#endif

  return res;
}

static QResult stateToNode(NodeBase *n, const std::string &st) {
  QResult res = QResult::ok();
#if USE_CEREAL
  std::istringstream stream(st);
  {
    cereal::JSONInputArchive archive(stream);
    std::vector<NodeBase *> usedNodes;

    NodeStateAPI(n, usedNodes).serialize(archive);

    // need delete archive to flush
  }

#else
#error "not supported"
#endif

  return res;
}

static std::string apiStateToString(APIInstanceBase *api) {

  std::string res;

#if USE_CEREAL
  std::ostringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    archive(cereal::make_nvp("members", SerializableAPI(api)));
    // need delete archive to flush
  }
  res = stream.str();
#else
  std::vector<std::string> tmp;
  for (const auto &m : api.members) {
    tmp.push_back(m.first + ":" + m.second->toString(&obj));
  };
  if (tmp.size())
    res += std::string(res.size() ? "," : "") + "members:{" +
           StringHelpers::joinIntoString(tmp) + "}";
#endif

  return res;
}

} // namespace APISerializer

#endif // USE_SERIALIZER
