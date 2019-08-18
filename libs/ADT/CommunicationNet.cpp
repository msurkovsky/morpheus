
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
  // UnresolvedConnect

  void UnresolvedConnect::close_connect(CommunicationNet &ctx_cn,
                                        AccessCommPlaceFnTy get_place,
                                        string arc_expr) {
    assert (acn_ != nullptr &&
            "Trying to close connection with unspecified AddressableCN.");
    assert (incomplete_edge.endpoint &&
            "IncompleteEdge has to be set with non-null endpoint.");

    ctx_cn.add_edge((acn_->*get_place)(), *incomplete_edge.endpoint, arc_expr,
                    incomplete_edge.category, incomplete_edge.type);
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
  void CommunicationNet::resolve_unresolved() {
    std::vector<UnresolvedPlace *> to_remove;

    // match unresolved places with unresolved transitions
    vector<UnresolvedTransition *> used;
    for (auto up_it = unresolved_places_.begin();
         up_it != unresolved_places_.end();
         up_it++) {

      auto &up = *up_it;
      auto matched_ut_it = std::find_if(
        unresolved_transitions_.begin(), unresolved_transitions_.end(),
        [&up](const auto &ut) { return &up->mpi_rqst == &ut->mpi_rqst; });
      if (matched_ut_it != unresolved_transitions_.end()) {
        auto &ut = *matched_ut_it;
        up->resolve(*this, up->place, ut->transition, ut->unresolved_connect);
        used.push_back(ut.get());
        to_remove.push_back(up.get());
      }
    }

    // check and eventually mark unresolved transitions
    std::sort(used.begin(), used.end());
    auto last = std::unique(used.begin(), used.end());
    size_t num_of_used = std::distance(used.begin(), last);

    if (num_of_used == unresolved_transitions_.size()) {
      // all unresolved transition has matched at least once
      unresolved_transitions_.clear();
    } else {
      used.erase(last, used.end());
      // find and remove the used ones
      auto end_of_unsed = unresolved_transitions_.end();
      while (!used.empty()) {
        UnresolvedTransition *ut_used = used.back();
        used.pop_back();

        end_of_unsed = std::remove_if(unresolved_transitions_.begin(), end_of_unsed,
                                      [ut_used](const auto &ut) { return ut.get() == ut_used; });
      }
      unresolved_transitions_.erase(unresolved_transitions_.begin(), end_of_unsed);

      // expose unused unresolved transitions
      for (auto &ut : unresolved_transitions_) {
        ut->transition.highlight_color = "#ff0000";
      }
    }

    // remove resolved places
    auto remove_from_it = unresolved_places_.end();
    while (!to_remove.empty()) {
      UnresolvedPlace *up_rm = to_remove.back();
      to_remove.pop_back();
      remove_from_it = std::remove_if(unresolved_places_.begin(), remove_from_it,
                                      [up_rm](const auto &up) { return up.get() == up_rm; });
    }
    unresolved_places_.erase(remove_from_it, unresolved_places_.end());

    // check and eventually mark unresolved places
    for (auto &up : unresolved_places_) {
      up->place.highlight_color = "#ff0000";
    }
  }

  void CommunicationNet::collapse() {
    CommunicationNet tmp_cn;
    collapse_topdown(places_, tmp_cn, &CommunicationNet::places_);
    collapse_topdown(transitions_, tmp_cn, &CommunicationNet::transitions_);
    std::swap(tmp_cn, *this);

    tmp_cn.clear();
    collapse_bottomup(places_, tmp_cn, &CommunicationNet::places_);
    collapse_bottomup(transitions_, tmp_cn, &CommunicationNet::transitions_);
    std::swap(tmp_cn, *this);

    reduce_parallel_paths();

    reduce_redundant_edges(collect_all_edges());
  }

  void CommunicationNet::takeover(CommunicationNet cn) {
    takeover_(places_, cn.places());
    takeover_(transitions_, cn.transitions());
    takeover_(unresolved_places_, cn.unresolved_places());
    takeover_(unresolved_transitions_, cn.unresolved_transitions());
  }

} // end of communication net (cn) namespace

