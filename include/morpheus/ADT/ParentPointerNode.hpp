
//===----------------------------------------------------------------------===//
//
// ParentPointerNode
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_PPNODE
#define MRPH_PPNODE

#include <memory>
#include <optional>

#include "llvm/Support/raw_ostream.h"

template<typename T>
class PPNode {

public:
  T data;
  std::shared_ptr<PPNode<T>> parent;

  explicit PPNode(const T &data) : data(data) { }
  PPNode(PPNode &&node) = default;
  PPNode() = delete;
  PPNode(const PPNode &node) = delete;

  static std::shared_ptr<PPNode<T>> create(const T &data) {
    return std::make_shared<PPNode<T>>(data);
  }
};

namespace llvm {
  template<typename T>
  raw_ostream &operator<< (raw_ostream &out, const PPNode<T> &node) {
    out << "Node(" << node.data << ")";
    if (node.parent) {
      out << " -> " << *node.parent;
    }
    return out;
  }
}

#endif // MMRPH_PPNODE
