
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

  // # protected
  bool CommunicationNet::remove(NetElement &elem) {
    if (elem.get_element_type() == "place_t") {
      return remove(static_cast<Place &>(elem));
    } else if (elem.get_element_type() == "transition_t") {
      return remove(static_cast<Transition &>(elem));
    } else {
      assert(false && "Unknown type of the net element.");
    }
    return false;
  }

  // + public methods
  void CommunicationNet::collapse() {
    CommunicationNet tmp_cn;

    collapse(places_, tmp_cn, &CommunicationNet::places_);
    collapse(transitions_, tmp_cn, &CommunicationNet::transitions_);

    std::swap(tmp_cn, *this);

    reduce_parallel_paths();
  }

  void CommunicationNet::takeover(CommunicationNet cn) {
    takeover_(places_, cn.places());
    takeover_(transitions_, cn.transitions());
  }

} // end of communication net (cn) namespace

