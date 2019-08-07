
#ifndef MRPH_UTILS_H
#define MRPH_UTILS_H

#include <string>
#include <vector>
#include <sstream>

namespace {

template<typename T>
void store_if_not(std::vector<T> &values, const T &v, T excl) {
  if (v != excl) {
    values.push_back(v);
  }
}

template<typename T>
std::string pp_vector(const std::vector<T> &values,
                      std::string delim=",",
                      std::string lbracket="",
                      std::string rbracket="") {
  std::ostringstream oss;
  if (values.empty()) {
    return "";
  }

  if (values.size() == 1) {
    oss << values[0];
    return oss.str();
  }

  // more than one value
  oss << lbracket;
  auto it = values.begin();
  oss << *it; it++;
  for (; it != values.end(); it++) {
    oss << delim << *it;
  }
  oss << rbracket;
  return oss.str();
}

} // end of anonymous namespace

#endif // MRPH_UTILS_H
