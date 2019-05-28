
//===----------------------------------------------------------------------===//
//
// ParentPointerNode
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_PPNODE_H
#define MRPH_PPNODE_H

#include <memory>
#include <optional>

#include "llvm/Support/raw_ostream.h"

template<typename T>
class PPNode {

  unsigned depth;
  std::shared_ptr<PPNode<T>> parent;

public:
  T data;

  static std::shared_ptr<PPNode<T>> create(const T &data) {
    return std::make_shared<PPNode<T>>(data);
  }

  explicit PPNode(const T &data) : depth(0), data(data) { }
  PPNode(PPNode &&node) = default;
  PPNode() = delete;
  PPNode(const PPNode &node) = delete;

  void set_parent(const std::shared_ptr<PPNode<T>> &parent) {
    this->parent = parent;
    depth = parent->depth + 1;
  }

  std::shared_ptr<PPNode<T>> get_parent() const {
    return parent;
  }

  unsigned get_depth() const {
    return depth;
  }
};

namespace llvm {
  template<typename T>
  raw_ostream &operator<< (raw_ostream &out, const PPNode<T> &node) {
    out << "Node(" << node.data << ")";
    if (node.get_parent()) {
      out << " -> " << *node.get_parent();
    }
    return out;
  }
}

#endif // MMRPH_PPNODE_H
