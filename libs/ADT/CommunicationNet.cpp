
#include "morpheus/ADT/CommunicationNet.hpp"
#include "morpheus/Formats/PlainText.hpp"

#include <algorithm>
#include <sstream>

namespace cn {

  using namespace llvm;

  Identifiable::ID Identifiable::generate_id() {
    static unsigned int id = 0;
    return std::to_string(++id);
  }

  // ---------------------------------------------------------------------------
  // CommunicationNet

  void CommunicationNet::collapse() {
    CommunicationNet tmp_cn;

    collapse(places_, tmp_cn, &CommunicationNet::places_);
    collapse(transitions_, tmp_cn, &CommunicationNet::transitions_);

    // TODO: remove freely hanging elements (22.7. (?) - it is not so clear if those should be removed.)
    std::swap(tmp_cn, *this);
  }

} // end of communication net (cn) namespace

