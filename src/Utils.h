#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace StringHelpers {

template <typename T>
std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value, std::string>
toString(const T &v) {
  return std::to_string(v);
};
template <typename T>
std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, std::string>
toString(const T &v) {
  return std::string(v);
}

// template <typename T>
// std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value, T>
// fromString(const std::string &v) {
//   return std::to_string(v);
// };

template <typename T>
// std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, T>
T fromString(const std::string &v) {
  T res;
  std::istringstream(v) >> res;
  return res;
  // return std::string(v);
}

// template <> std::string toString<float>(const float &v) {
//   return std::to_string(v);
// }

template <typename AT, typename ET = std::remove_reference_t<
                           decltype(*std::begin(std::declval<AT &>()))>>
static std::string joinIntoString(
    const AT &alist, const std::string &delim = ",",
    std::function<std::string(const ET &e)> toStr = [](const ET &e) {
      return e;
    }) {

  bool isFirst = true;
  std::string res;
  for (const auto &e : alist) {
    if (!isFirst) {
      res += delim;
    }
    isFirst = false;
    res += toStr(e);
  }
  return res;
}

static std::vector<std::string> splitString(const std::string &s,
                                            const char delim) {
  std::vector<std::string> res;
  std::string tmp;
  for (int i = 0; i < s.size(); i++) {
    auto c = s[i];
    if (c == delim) {
      if (tmp.size() && (i != s.size() - 1)) { // force push manually
        res.push_back(tmp);
        tmp.clear();
      }
    } else {
      tmp += s[i];
    }
  }
  if (tmp.size()) { //  push manually
    res.push_back(tmp);
  }
  return res;
}

namespace JSONSchema {
template <typename T,
          std::enable_if_t<std::is_same<T, std::string>::value> * = nullptr>
std::string getTypeName() {
  return "s";
};

template <typename T, std::enable_if_t<std::is_same<T, int>::value> * = nullptr>
std::string getTypeName() {
  return "i";
};

template <typename T,
          std::enable_if_t<std::is_same<T, float>::value> * = nullptr>
std::string getTypeName() {
  return "f";
};

template <typename T,
          std::enable_if_t<std::is_same<T, double>::value> * = nullptr>
std::string getTypeName() {
  return "f";
};

template <typename T,
          std::enable_if_t<std::is_same<T, bool>::value> * = nullptr>
std::string getTypeName() {
  return "b";
};

} // namespace JSONSchema

} // namespace StringHelpers
