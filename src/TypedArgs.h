#pragma once
#include "Utils.h"
#include "log.h"
#include <string>

// using std::string;
#include <vector>
// using std::vector;
#include <memory>
#include <tuple>
#include <typeindex>
typedef std::type_index TypeID;

struct Undefined {};
struct TypeOfBase {
  virtual TypeID value() const { return typeid(Undefined); }
  virtual std::string name() const { return "Undefined"; }
};
template <typename T> struct TypeOf {
  static TypeID value() { return typeid(T); }
  static std::string name() { return value().name(); }
};

template <> std::string TypeOf<std::string>::name() { return "s"; }
template <> std::string TypeOf<void>::name() { return "v"; }

template <typename T> struct TypeOfImpl : public TypeOfBase {
  TypeID value() const override { return TypeOf<T>::value(); }
  std::string name() const override { return TypeOf<T>::name(); }
};

struct TypedArgBase {
  typedef std::unique_ptr<TypedArgBase> UPtr;
  static const TypedArgBase &none() {
    static TypedArgBase n;
    return n;
  };

  TypedArgBase(const TypeOfBase &typ = TypeOfBase()) : type(typ) {
    if (type.value() == TypeOf<TypedArgBase>::value()) {
      PRINTLN("!!!creating base class");
    }
  }

public:
  TypedArgBase(TypedArgBase &&) = delete;
  // TypedArgBase &operator=(const TypedArgBase &) = default;
  virtual ~TypedArgBase() = default;
  const TypeOfBase &type = {};
  template <typename T> T get() const;
  template <typename T> T &getRef();
  template <typename T> bool is() const;
  template <typename T> bool set(const T &) {
    PRINTLN("setting empty");
    return false;
  }
  virtual std::unique_ptr<TypedArgBase> clone() const {
    PRINTLN("cloning empty");
    return std::make_unique<TypedArgBase>();
  }
  virtual bool fromString(const std::string &s) { return false; }
  virtual std::string toString() const {
    PRINTLN("stringing empty");
    return "undefined";
  }
};

template <typename T> struct isTypedArgBase {
  static constexpr bool ptr =
      std::is_same<TypedArgBase *, std::decay_t<T>>::value;
  static constexpr bool ref =
      std::is_same<TypedArgBase, std::decay_t<T>>::value && !ptr;
  static constexpr bool uptr =
      std::is_same<std::unique_ptr<TypedArgBase>, std::decay_t<T>>::value;

  static constexpr bool any = ref || uptr || ptr;
};

template <typename T, std::enable_if_t<!isTypedArgBase<T>::any, bool> = true>
struct TypedArg : public TypedArgBase {
  TypedArg(const T &source) : TypedArgBase(TypeOfImpl<T>()), v(source) {}
  ~TypedArg() {}
  const T &get() const { return v; }
  T &getRef() { return v; }
  bool set(const T &e) {
    v = e;
    return true;
  }
  std::unique_ptr<TypedArgBase> clone() const override {
    return std::unique_ptr<TypedArgBase>(new TypedArg<T>(v));
  }
  std::string toString() const override {
    return StringHelpers::toString<T>(v);
  }

  T v;
};

template <> struct TypedArg<void> : public TypedArgBase {

  TypedArg() : TypedArgBase(TypeOfImpl<void>()) {}
  ~TypedArg() {}
  std::unique_ptr<TypedArgBase> clone() const override {
    return std::unique_ptr<TypedArgBase>(new TypedArg<void>());
  }
  std::string toString() const override { return "void"; }
};

template <typename T> bool TypedArgBase::is() const {
  return dynamic_cast<const TypedArg<T> *>(this) != nullptr;
}

template <typename T> T &TypedArgBase::getRef() {
  PRINT("gettingRef : ");
  std::string tname(TypeOf<T>::name());
  PRINT(tname.c_str());
  if (auto d = dynamic_cast<TypedArg<T> *>(this)) {
    PRINTLN("-> success");
    return d->getRef();
  }
  static T defaultEmpty = {};
  PRINTLN("!! failed get ref : will return 0");
  return defaultEmpty;
}
template <typename T> T TypedArgBase::get() const {
  if (auto d = dynamic_cast<const TypedArg<T> *>(this)) {
    return d->get();
  }
  std::string tname(TypeOf<T>::name());
  PRINT("casting : ");
  PRINTLN(tname.c_str());
  // PRINT(" from : ");
  // try {
  //   PRINTLN(this->type.name().c_str());
  // } catch (std::exception e) {
  //   PRINT("exception : ");
  //   PRINTLN(e.what());
  // }
  // if (std::is_floating_point<T>::value) {
  if (auto d = dynamic_cast<const TypedArg<int> *>(this)) {

    return T(d->get());
  }
  // }
  // if (std::is_integral<T>::value) {
  // PRINTLN("try cast int");
  if (auto d = dynamic_cast<const TypedArg<float> *>(this)) {
    return T(d->get());
  }
  if (auto d = dynamic_cast<const TypedArg<double> *>(this)) {
    return T(d->get());
  }
  if (auto d = dynamic_cast<const TypedArg<bool> *>(this)) {
    return T(d->get() ? 1 : 0);
  }
  if (auto d = dynamic_cast<const TypedArg<std::string> *>(this)) {
    return T(std::stold(d->get()));
  }
  // }
  PRINTLN("!! failed try cast from int/float / double / bool /string : will "
          "return 0");
  return {};
}

template <> std::string TypedArgBase::get() const { return this->toString(); }
struct TypedArgList {

  typedef std::unique_ptr<TypedArgBase> Ptr;
  typedef std::vector<Ptr> ArgVecType;
  static const TypedArgList &empty() {
    static TypedArgList e;
    return e;
  };
  TypedArgList() = default;
  TypedArgList(TypedArgList &&o) : args(std::move(o.args)) {}
  template <typename... Args> TypedArgList(const Args &...a) {
    appendOne(a...);
  }
  template <typename T, typename... Args>
  std::enable_if_t<!isTypedArgBase<T>::any, void> appendOne(const T &e,
                                                            const Args &...a) {
    // PRINTLN("appending val non Typed");
    args.push_back(std::make_unique<TypedArg<T>>(e));
    appendOne(a...);
  }

  template <typename T, typename... Args>
  std::enable_if_t<isTypedArgBase<T>::uptr, void> appendOne(const T &e,
                                                            const Args &...a) {
    // PRINTLN("appending uptr typed");
    if (e.get()) {
      args.push_back(e->clone());
    } else {
      PRINTLN("!!appending empty ptr");
    }
    appendOne(a...);
  }
  template <typename T, typename... Args>
  std::enable_if_t<isTypedArgBase<T>::ref, void> appendOne(const T &e,
                                                           const Args &...a) {
    // PRINTLN("appending ref typed");
    args.push_back(e.clone());
    appendOne(a...);
  }

  template <typename T>
  std::enable_if_t<!isTypedArgBase<T>::any, void> appendOne(const T &e) {
    // PRINTLN("appending last  non Typed");
    args.push_back(std::make_unique<TypedArg<T>>(e));
  }

  template <typename T>
  std::enable_if_t<isTypedArgBase<T>::uptr, void> appendOne(const T &e) {
    // PRINTLN("appending  last uptr typed");
    if (e.get()) {
      args.push_back(e->clone());
    } else {
      PRINTLN("!!appending empty ptr");
    }
  }

  template <typename T>
  std::enable_if_t<isTypedArgBase<T>::ref, void> appendOne(const T &e) {
    // PRINTLN("appending  last ref typed");
    args.push_back(e.clone());
  }

  // template <typename T>
  // TypedArgList(const T &e) : args({std::make_unique<TypedArg<T>>(e)}) {}
  // TypedArgList(TypedArgBase *b) { args.push_back(Ptr(b)); }
  // TypedArgList(Ptr &&b) { args.push_back(std::move(b)); }
  template <typename T> void set(const T &e) { args = {e}; }
  template <typename T> void set(int i, const T &e) {
    if (i >= args.size()) {
      args.resize(i + 1);
    }
    args[i] = {e};
  }

  typename ArgVecType::iterator begin() { return args.begin(); }
  typename ArgVecType::const_iterator begin() const { return args.begin(); }
  typename ArgVecType::const_iterator cbegin() const { return begin(); }
  typename ArgVecType::iterator end() { return args.end(); }
  typename ArgVecType::const_iterator end() const { return args.end(); }
  typename ArgVecType::const_iterator cend() const { return end(); }

  const TypedArgBase *operator[](int i) const { return args[i].get(); }
  TypedArgBase *operator[](int i) { return args[i].get(); }

  void resize(int i) { args.resize(i); }
  int size() const { return args.size(); }
  ArgVecType args;
};

namespace TupleFiller {

template <typename T>
using BaseType = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename Tuple, int I>
using TupleType = typename std::tuple_element<I, Tuple>::type;

template <typename Tuple, int I>
using BaseTupleType = BaseType<TupleType<Tuple, I>>;

// template <typename Tuple, int I>
// using TupleTypeIsVoid = std::is_void<TupleType<Tuple, I>>::value;

// TupleFiller(Tuple &t, TypedArgList a) : args(a), tuple(t) {}

// template <typename Tuple, int I>
//     typename std::enable_if_t < (I > 0),
//     std::enable_if_t<std::is_void<typename, void>> fillOne(
//         Tuple &tuple, const TypedArgList &args) {
//   fillOne<Tuple, I - 1>(tuple);
// }

// template <typename Tuple, int I>
// inline typename std::enable_if_t<
//     (I <= 0),
//     std::enable_if_t<
//         std::is_void<typename std::tuple_element<I, Tuple>::type>::value,
//         void>>
// fillOne(Tuple &tuple, const TypedArgList &args) {}

template <typename Tuple, int I>
inline typename std::enable_if_t<(I < 0), void>
fillOne(Tuple &tuple, const TypedArgList &args) {}

template <typename Tuple, int I>
inline typename std::enable_if_t<(I == 0), void>
fillOne(Tuple &tuple, const TypedArgList &args) {
  std::get<0>(tuple) = args[0]->get<BaseTupleType<Tuple, I>>();
}

template <typename Tuple, int I>
inline typename std::enable_if_t<(I > 0), void>
fillOne(Tuple &tuple, const TypedArgList &args) {
  std::get<I>(tuple) = args[I]->get<BaseTupleType<Tuple, I>>();
  fillOne<Tuple, (I - 1)>(tuple, args);
}

///////////////

template <typename... Args>
std::tuple<Args...> fillTuple(const TypedArgList &args, bool &valid) {
  if (args.size() != sizeof...(Args)) {
    valid = false;
    return {};
  }
  valid = true;
  std::tuple<Args...> t;
  // fillOneInTuple<std::tuple<Args...>, sizeof...(Args) - 1>(t);
  TupleFiller::fillOne<std::tuple<Args...>, sizeof...(Args) - 1>(t, args);
  return t;
}

}; // namespace TupleFiller
// template <typename Tuple>
// void TypedArgList::fillOneInTuple<Tuple, 0>(Tuple &tuple) const {
//   // constexpr int i = std::tuple_size<Tuple>::value - sizeof...(Remaining);
//   std::get<0>(tuple) =
//       args[0].get<typename std::tuple_element<0, Tuple>::type>();
// }

struct QResult {
  QResult() = default;
  QResult(QResult &&o) = default;
  QResult(TypedArgBase *r) : valid(true), res(r){};
  QResult(TypedArgBase::UPtr &&r) : valid(true), res(std::move(r)){};

  explicit QResult(bool hasErr) : valid(hasErr){};
  operator bool() const { return valid; }
  // ~QResult() = default;
  // template <typename T> QResult(T r) : valid(true), res(r) {}

  template <typename T> void set(T r) {
    valid = true;
    res = std::make_unique<TypedArg<T>>(r);
    errMsg.clear();
  }

  // void setResult(const TypedArgBase &r) {
  //   valid = true;
  //   res = r;
  //   errMsg.clear();
  // }

  static QResult err(const std::string &err) { return QResult(err); }

  template <typename T> static QResult ok(const T &r) {
    return QResult(std::make_unique<TypedArg<T>>(r));
  }
  static QResult ok() { return QResult(true); };
  std::string toString() {
    return valid ? (res ? res->toString() : "valid but empty") : errMsg;
  }
  bool valid = false;
  TypedArgBase::UPtr res = {}; // TypedArgBase::none();
  std::string errMsg = {};

private:
  QResult(const std::string &err) : valid(false), errMsg(err){};
};
// static_assert(std::is_aggregate<QResult>::value);
