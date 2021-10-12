#pragma once
#include "APIBase.h"


#if USE_CEREAL
#include <cereal/archives/json.hpp>
#endif

struct APIInstanceBase {

  virtual ~APIInstanceBase() = default;
  APIInstanceBase(const APIInstanceBase &) = delete;
  APIInstanceBase() = default;
  virtual APIBase *getAPI() = 0;
  virtual QResult get(const Identifier &n) = 0;
  virtual QResult set(const Identifier &n, const TypedArgList &args) = 0;
  virtual QResult call(const Identifier &n, const TypedArgList &args) = 0;
  virtual bool canGet(const Identifier &id) const = 0;
  virtual bool canSet(const Identifier &id) const = 0;
  virtual bool canCall(const Identifier &id) const = 0;
  virtual MemberInstanceBase *getMemberInstance(const Identifier &n) = 0;
  // virtual std::string toString() const = 0;
  // virtual QResult fromString(const std::string &) = 0;
};

template <typename C> struct APIInstance : public APIInstanceBase {

  APIInstance(C *o, API<C> *a) : obj(o), api(a) {}
  APIBase *getAPI() override { return api; }
  QResult get(const Identifier &n) override {
    DBGRESOLVE("try get ");
    DBGRESOLVE((long)(void *)this);
    DBGRESOLVE(" ");
    DBGRESOLVE(n.c_str());
    DBGRESOLVE(" : ");
    if (auto *getter = getOrNull(api->getters, n)) {
      DBGRESOLVE(", getter,");
      QResult r;
      bool success = tryGet<C, float>(obj, getter, r) ||
                     tryGet<C, int>(obj, getter, r) ||
                     tryGet<C, bool>(obj, getter, r) ||
                     tryGet<C, std::string>(obj, getter, r);
      if (success) {
        DBGRESOLVELN(" -> found");
        return r;
      }
    }
    if (auto *getter = getOrNull(api->members, n)) {
      DBGRESOLVE(", members,");
      QResult r;
      bool success = tryGet<C, float>(obj, getter, r) ||
                     tryGet<C, int>(obj, getter, r) ||
                     tryGet<C, bool>(obj, getter, r) ||
                     tryGet<C, std::string>(obj, getter, r);
      if (success) {
        DBGRESOLVELN(" -> found");
        return r;
      }
    }
    DBGRESOLVELN(" -> not found!!!!");
    return QResult::err("cannot get ");
  }

  QResult set(const Identifier &n, const TypedArgList &args) override {
    DBGRESOLVE("try set  ");
    DBGRESOLVE(n.c_str());
    if (auto *member = getOrNull(api->members, n)) {
      if (args.size() == 1) {
        auto *v = args[0];
        bool success = trySet<C, float>(obj, member, v) ||
                       trySet<C, int>(obj, member, v) ||
                       trySet<C, bool>(obj, member, v) ||
                       trySet<C, std::string>(obj, member, v);
        if (success) {
          DBGRESOLVELN("-> success");
        } else {
          DBGRESOLVELN("-> error");
        }
        return QResult(!success);
      }
      DBGRESOLVELN("-> error 1");
      return QResult::err("cannot set wrong num args");
    }
    DBGRESOLVELN("-> error 2");
    return QResult::err("cannot set no member");
  }

  QResult call(const Identifier &n, const TypedArgList &args) override {
    if (auto *fun = api->getFunction(n)) {
      return tryCall<C>(obj, *fun, args);
    }
    DBGRESOLVELN("-> error 2");
    return QResult::err("no function found");
  }

  bool canGet(const Identifier &id) const override { return api->canGet(id); }
  bool canSet(const Identifier &id) const override { return api->canSet(id); }
  bool canCall(const Identifier &id) const override { return api->canCall(id); }

  // Generic member functions

  template <typename T> struct MemberIImpl : public MemberInstanceT<T> {
    MemberIImpl(MemberValue<C, T> *m, C *owner) : m(m), owner(owner) {}
    T get() const override { return m->get(*owner); };
    void set(const T &v) override { m->set(*owner, v); };
    T &getRef() override { return m->getRef(*owner); }

    MemberValue<C, T> *m;
    C *owner;
  };
  MemberInstanceBase *getMemberInstance(const Identifier &id) override {
    auto *resolved = getOrNull(api->members, id);
    if (resolved) {
      if (auto m = getMemberInstanceT<float>(resolved))
        return m;
      if (auto m = getMemberInstanceT<int>(resolved))
        return m;
      if (auto m = getMemberInstanceT<std::string>(resolved))
        return m;
      if (auto m = getMemberInstanceT<double>(resolved))
        return m;
      if (auto m = getMemberInstanceT<bool>(resolved))
        return m;
    }
    return nullptr;
  };
  template <typename T> MemberIImpl<T> *getMemberInstanceT(MemberBase *m) {
    auto *resolved = dynamic_cast<MemberValue<C, T> *>(m);
    if (resolved) {
      return new MemberIImpl<T>(resolved, obj);
    }
    return nullptr;
  };
  C *obj;
  API<C> *api;
};

// defines both API and instanciation
template <typename C>
struct APIAndInstance : public API<C>, public APIInstance<C> {
  APIAndInstance(C *obj) : APIInstance<C>(obj, (API<C> *)(this)) {}
};
