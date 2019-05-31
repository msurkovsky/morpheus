
#ifndef MRPH_UTILS_H
#define MRPH_UTILS_H

#include <string>
#include <vector>
#include <sstream>

template<typename T>
void store_if_not(std::vector<T> &values, const T &v, T excl) {
  if (v != excl) {
    values.push_back(v);
  }
}

template<typename T>
std::string format_tuple(const std::vector<T> &values) {
  std::ostringstream str;
  if (values.size() == 1) {
    str << values[0];
  } else if (values.size() > 1) {
    str << "(";
    auto it = values.begin();
    str << *it; it++;
    for (; it != values.end(); it++) {
      str << ", " << *it;
    }
    str << ")";
    return str.str();
  }
  return str.str();
}

#endif // MRPH_UTILS_H
