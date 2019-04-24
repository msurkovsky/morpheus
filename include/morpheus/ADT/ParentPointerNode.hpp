
//===----------------------------------------------------------------------===//
//
// ParentPointerNode
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_PPNODE
#define MRPH_PPNODE

#include <memory>
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

template<typename T>
class PPNode {
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, const PPNode<T> &);

public:
  T data;
  std::shared_ptr<PPNode<T>> parent;
  llvm::StringRef metadata = "X";

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
    } else {
      out << ";";
    }
    return out;
  }
}
#endif // MMRPH_PPNODE
