#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct NodeBase {
  typedef std::string Id;
  NodeBase() : parentNode(nullptr) {}
  virtual ~NodeBase() = default;
  virtual bool setup() { return true; };
  virtual void handle(){};
  virtual bool hasChilds() const = 0;
  void addChild(const Id &i, NodeBase *c) {
    c->parentNode = this;
    c->idInParent = i;
    childNodes.push_back(c);
    addChildInternal(i, c);
  }
  virtual void addChildInternal(const Id &i, NodeBase *c) = 0;
  virtual NodeBase *getChild(const Id &c) = 0;
  std::vector<std::string> getAddress() {
    std::vector<std::string> res;
    auto *insp = this;
    int maxD = 99;
    std::vector<NodeBase *> seen;
    while (insp && (maxD--)) {
      if (!insp->idInParent.size()) {
        DBGRESOLVELN("!!! empty id in parent");
      }
      if (insp == insp->parentNode) {
        res.push_back(".");
        break;
      }
      if (std::find(seen.begin(), seen.end(), insp) != seen.end()) {
        res.push_back("[cycl]");
        break;
      }
      seen.push_back(insp);
      res.push_back(insp->idInParent);
      insp = insp->parentNode;
    }
    if (res.size())
      std::reverse(res.begin(), res.end());
    return res;
  }
  std::pair<NodeBase *, int> resolveAddr(const std::vector<std::string> &addr,
                                         int fromI = 0) {
    auto insp = this;
    int lastValidIdx = insp ? fromI : -1;
    if (lastValidIdx < addr.size() - 1)
      DBGRESOLVE("[rslv] try getting child : ");
    while (insp) {
      if (lastValidIdx < addr.size() - 1) {
        DBGRESOLVE(addr[lastValidIdx].c_str());
        DBGRESOLVE(" (");
        DBGRESOLVE(String(lastValidIdx));
        DBGRESOLVE(") ");
        auto *ninsp = insp->getChild(addr[lastValidIdx]);
        if (ninsp) {
          DBGRESOLVE("found");
          insp = ninsp;
          lastValidIdx++;
        } else {
          DBGRESOLVE("not found");
          lastValidIdx--;
          break;
        }
        DBGRESOLVE(" // ");
      } else {
        break;
      }
    }
    DBGRESOLVELN("");
    return {insp, lastValidIdx};
  }
  Id idInParent;
  NodeBase *parentNode = nullptr;

  std::vector<NodeBase *> childNodes;
};

// helpers to tag leaf classes
struct LeafNode : public NodeBase {
  bool hasChilds() const override { return false; }
  void addChildInternal(const Id &i, NodeBase *c) override {}
  NodeBase *getChild(const Id &c) override { return nullptr; }
};

#if 0
struct NodeRefBase : public NodeBase {
  virtual ~NodeRefBase() = default;
  virtual NodeBase *getRef() = 0;
};
template <typename T
          //   ,std::enable_if_t<std::is_base_of<NodeBase, T>::value> * =
          //   nullptr
          >
struct NodeRef : public NodeRefBase {
  NodeRef(T *ref, bool own = false) : ref(ref), own(own) {}
  ~NodeRef() {
    if (own) {
      delete ref;
    }
  }

  NodeBase *getRef() override { return dynamic_cast<NodeBase *>(ref); }
  bool hasChilds() const override {
    // if (auto r = dynamic_cast<NodeBase *>(ref)) {
    //   return r->hasChilds();
    // }
    return false;
  }
  void addChildInternal(const Id &i, NodeBase *c) override {
    // if (auto r = dynamic_cast<NodeBase *>(ref)) {
    //   return r->addChildInternal(i, c);
    // }
  }
  NodeBase *getChild(const Id &c) override {
    // if (auto r = dynamic_cast<NodeBase *>(ref)) {
    //   return r->getChild(i);
    // }
    return nullptr;
  }
  T *ref;
  bool own;
};

#endif

struct ArrayNode : public NodeBase {
  ArrayNode() : NodeBase() {}
  bool hasChilds() const override { return v.size(); }
  void addChildInternal(const Id &i, NodeBase *c) override { v.push_back(c); }
  NodeBase *getChild(const Id &c) override {
    size_t i = std::stoi(c);
    if ((i >= 0) && i < v.size()) {
      return v[i];
    }
    return nullptr;
  }
  std::vector<NodeBase *> v;
};

struct MapNode : public NodeBase {
  MapNode() : NodeBase() {}
  bool hasChilds() const override { return m.size(); }
  void addChildInternal(const Id &i, NodeBase *c) override { m.emplace(i, c); }
  NodeBase *getChild(const Id &c) override {
    auto it = m.find(c);
    if (it != m.end()) {
      return it->second;
    }
    return nullptr;
  }
  std::map<Id, NodeBase *> m;
};

template <typename T> static T *getLinkedObj(NodeBase *n) {
  // if (auto r = dynamic_cast<NodeRefBase *>(n)) {
  //   auto *ref = r->getRef();
  //   auto casted = dynamic_cast<T *>(ref);
  //   if (!ref) {
  //     DBGRESOLVELN("no obj linked");
  //   } else if (!casted) {
  //     DBGRESOLVELN("linked but can't cast");
  //   }
  //   return casted;
  // } else
  if (auto r = dynamic_cast<T *>(n)) {
    return r;
  }
  return nullptr;
}
