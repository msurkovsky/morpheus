
#ifndef MRPH_UTILS_H
#define MRPH_UTILS_H

#include "llvm/IR/Value.h"

#include <string>
#include <vector>
#include <functional>

namespace Utils {

template<typename T>
void store_if_not(std::vector<T> &values, const T &v, T excl);

template<typename T>
std::string pp_vector(const std::vector<T> &values,
                      std::string delim=",",
                      std::string lbracket="",
                      std::string rbracket="",
                      std::function<bool(const T &)>
                      filter_fn=[] (const T &) { return true; });

std::string value_to_type(const llvm::Value &v, bool return_constant=true);

std::string value_to_str(const llvm::Value &v, std::string name, bool return_constant=true);

std::string compute_envelope_type(llvm::Value const *src,
                                  llvm::Value const *dest,
                                  const llvm::Value &tag,
                                  std::string delim=",",
                                  std::string lbracket="",
                                  std::string rbracket="");

std::string compute_envelope_value(llvm::Value const *src,
                                   llvm::Value const *dest,
                                   const llvm::Value &tag,
                                   bool include_constant=true,
                                   std::string delim="",
                                   std::string lbracket="",
                                   std::string rbracket="");

std::string compute_data_buffer_type(const llvm::Value &);

std::string compute_data_buffer_value(const llvm::Value &,
                                      const llvm::Value &);

std::string compute_msg_rqst_value(llvm::Value const *src,
                                   llvm::Value const *dest,
                                   const llvm::Value &tag,
                                   std::string buffered,
                                   std::string delim="");

}

#endif // MRPH_UTILS_H
