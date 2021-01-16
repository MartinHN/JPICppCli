#pragma once

#if USE_SERIALIZER
#if USE_CEREAL
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>

#endif

#include "APIInstance.h"
#include <algorithm>
#include <string>

namespace APISerializer {

#if USE_CEREAL

#define SKIP_OBJECT_DEFS 1
#define SKIP_API_DEFS 1
template <typename T> struct SimpleMap {
  SimpleMap(T &ob) : ob(ob) {}
  template <class AR> void serialize(AR &ar) const {
    for (const auto &m : ob) {
      ar(cereal::make_nvp(m.first, m.second));
    }
  }
  T &ob;
};

#if !SKIP_OBJECT_DEFS
template <typename T> struct TypeCont {
  template <typename... Args> TypeCont(Args... args) : ob(args...) {}
  template <class AR> void serialize(AR &ar) const {

    ar(cereal::make_nvp("type", std::string("object")));
    ar(cereal::make_nvp("properties", ob));
  }
  T ob;
};
#else
template <typename T> struct TypeCont : public T {
  template <typename... Args> TypeCont(Args... args) : T(args...) {}
};
#endif

struct APIMemberBase {
  APIMemberBase(MemberBase *m) : member(m) {}
  template <class AR> void serialize(AR &ar) const {
    bool s;
    s = chkAddM<std::string>(ar) || chkAddM<float>(ar) || chkAddM<bool>(ar) ||
        chkAddM<double>(ar) || chkAddM<int>(ar);
    if (!s) {
      PRINTLN("!!!serialization error");
    }
    for (auto &i : member->typeMeta) {
      ar(cereal::make_nvp(i.first, i.second));
    }
  }
  template <typename T, typename AR> bool chkAddM(AR &ar) const {
    if (auto *mv = dynamic_cast<const MemberBaseT<T> *>(member)) {
      ar(cereal::make_nvp("type", StringHelpers::JSONSchema::getTypeName<T>()));
      return true;
    }
    return false;
  }
  MemberBase *member;
};

struct APISchema {
  APISchema(APIBase *api) : api(api) {}
  template <class AR> void serialize(AR &ar) const {

    // members
    if (api->members.size()) {
      std::map<std::string, APIMemberBase> mMap = {};
      for (auto &m : api->members) {
        mMap.emplace(m.first, APIMemberBase(m.second));
      }
      ar(cereal::make_nvp("members",
                          TypeCont<SimpleMap<decltype(mMap)>>(mMap)));
    }
    // functions
    if (api->functions.size()) {
      std::map<std::string, std::string> fMap = {};
      for (auto &m : api->functions) {
        fMap.emplace(m.first, m.second->getSignature());
      }
      ar(cereal::make_nvp("functions",
                          TypeCont<SimpleMap<decltype(fMap)>>(fMap)));
    }

    // getter
    if (api->getters.size()) {
      std::map<std::string, std::string> gMap = {};
      for (auto &m : api->getters) {
        gMap.emplace(m.first, m.second->getTypeName());
      }
      ar(cereal::make_nvp("getters",
                          TypeCont<SimpleMap<decltype(gMap)>>(gMap)));
    }
  }

  APIBase *api;
};

struct APISchemaNodeSerializer {

  APISchemaNodeSerializer(NodeBase *node, std::vector<NodeBase *> *usedNodes)
      : node(node), usedNodes(usedNodes) {
    PRINT(" seen :  ");
    PRINTLN(node->idInParent.c_str());
    usedNodes->push_back(node);
  }

  template <class AR> void serialize(AR &ar) {
    if (auto apiI = dynamic_cast<APIInstanceBase *>(node)) {
#if SKIP_API_DEFS
      TypeCont<APISchema>(apiI->getAPI()).serialize(ar);
#else
      ar(cereal::make_nvp("api", TypeCont<APISchema>(*api)));
#endif
    }

    std::map<std::string, TypeCont<APISchemaNodeSerializer>> serCh;
    std::map<std::string, std::string> refToExisting;
    if (auto nm = dynamic_cast<MapNode *>(node)) {
      for (auto &c : nm->m) {
        PRINT("used");
        PRINTLN(usedNodes->size());
        if (std::find(usedNodes->begin(), usedNodes->end(), c.second) !=
            std::end(*usedNodes)) {
          refToExisting.emplace(c.first, StringHelpers::joinIntoString(
                                             c.second->getAddress(), "/"));

          PRINTLN("cyclic dep");
          continue;
        }
        PRINT("adding ");
        PRINTLN(c.first.c_str());

        serCh.emplace(c.first,
                      TypeCont<APISchemaNodeSerializer>(c.second, usedNodes));
      }
    }
    if (serCh.size()) {
      ar(cereal::make_nvp("childs", SimpleMap<decltype(serCh)>(serCh)));
    }
    if (refToExisting.size()) {
      ar(cereal::make_nvp("refs",
                          SimpleMap<decltype(refToExisting)>(refToExisting)));
    }
  }
  NodeBase *node;
  std::vector<NodeBase *> *usedNodes;
};
#endif

static std::string schemaFromNode(NodeBase *n) {
  std::string res;
#if USE_CEREAL
  std::ostringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    std::vector<NodeBase *> usedNodes;
#if !SKIP_OBJECT_DEFS
    archive(cereal::make_nvp("type", std::string("object")));
    archive(
        cereal::make_nvp("properties", APISchemaNodeSerializer(n, usedNodes)));
#else
    APISchemaNodeSerializer(n, &usedNodes).serialize(archive);

#endif
    // need delete archive to flush
  }
  res = stream.str();
#else
#error "not supported"
#endif

  return res;
}

} // namespace APISerializer

#endif // USE_SERIALIZER
