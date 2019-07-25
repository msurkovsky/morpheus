
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

    std::swap(tmp_cn, *this);
  }

  void CommunicationNet::takeover(CommunicationNet cn) {
    takeover_(places_, cn.places());
    takeover_(transitions_, cn.transitions());
  }

} // end of communication net (cn) namespace

