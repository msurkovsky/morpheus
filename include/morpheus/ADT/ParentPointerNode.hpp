
//===----------------------------------------------------------------------===//
//
// ParentPointerNode
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_PPNODE
#define MRPH_PPNODE

#include <memory>

template<typename T>
class PPNode {
  T data;

public:
  std::shared_ptr<PPNode<T>> parent;

  explicit PPNode(const T &data) : data(data) { }
  PPNode(PPNode &&node) = default;
  PPNode() = delete;
  PPNode(const PPNode &node) = delete;

  static std::shared_ptr<PPNode<T>> create(const T &data) {
    return std::make_shared<PPNode<T>>(data);
  }
};

#endif // MMRPH_PPNODE
