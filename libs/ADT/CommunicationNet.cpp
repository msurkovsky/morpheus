
#include "morpheus/ADT/CommunicationNet.hpp"

#include <algorithm>

namespace cn {

  using namespace llvm;

  Identifiable::ID Identifiable::generate_id() {
    static unsigned int id = 0;
    return std::to_string(++id);
  }

  void NetElement::print(raw_ostream &os) const {
    if (name.empty()) {
      os << this; // print pointer value
    } else {
      os << name;
    }
  }

  void Edge::print(raw_ostream &os) const {
    if (arc_expr.empty()) {
      os << startpoint << " -> " << endpoint;
    } else {
      os << startpoint << " --/ " << arc_expr << " /--> " << endpoint;
    }
  }

  void Place::print(raw_ostream &os) const {
    os << "P(" << get_id() << "): ";

    NetElement::print(os);

    os << "<";
    if (!type.empty()) {
      os << type;
    }
    os << ">";

    os << "[";
    if (!init_expr.empty()) {
      os << init_expr;
    }
    os << "]";
  }

  void Transition::print(raw_ostream &os) const {
    os << "T(" << get_id() << "): ";

    NetElement::print(os);

    os << "[";
    if (std::distance(guard.begin(), guard.end()) > 0) {
      auto it = guard.begin();
      for (; std::distance(it, guard.end()) > 1; it++) {
        os << *it << ", ";
      }
      os << *it;
    }
    os << "]";
  }

  // ---------------------------------------------------------------------------
  // CommunicationNet

  void CommunicationNet::print(raw_ostream &os) const {
    os << "CommunicationNet(" << get_id() << "):\n";

    os << "Places:\n";
    print_(places_, os, 2);

    os << "Transitions:\n";
    print_(transitions_, os, 2);

    os << "Input edges:\n";
    for (const auto &p : places_) {
      print_(p->leads_to, os, 2, EdgePredicate<REGULAR>());
    }

    os << "Outuput edges:\n";
    for (const Element<Transition> &t : transitions_) {
      print_(t->leads_to, os, 2, EdgePredicate<REGULAR>());
    }

    os << "CF edges: \n";
    for (const auto &p : places_) {
      print_(p->leads_to, os, 2, EdgePredicate<CONTROL_FLOW>());
    }
    for (const auto &t : transitions_) {
      print_(t->leads_to, os, 2, EdgePredicate<CONTROL_FLOW>());
    }
  }

  void CommunicationNet::collapse() {
    CommunicationNet tmp_cn;

    size_t p_idx = 0;
    while (p_idx < places_.size()) {
      Place &p = *places_[p_idx];
      if (p.type == "Unit" && p.leads_to.size() == 1) {

        Edge &e = *p.leads_to.back();
        if (e.endpoint.get_element_type() == "place_t") { // TODO: "place_t" move to the static variable into Place
          Place &endpoint = static_cast<Place&>(e.endpoint);
          if (endpoint.type == "Unit") {
            p_idx++; // skip the place and continue with another one
            continue;
          }
        }
      }

      // overtake place
      tmp_cn.add_place(move(places_[p_idx]));
      p_idx++;
    } // end of collapsing `P<Unit> --( cf_edge )--> P<Unit>` pattern

    // takes over all transitions
    tmp_cn.takeover_(tmp_cn.transitions_, transitions());

    std::swap(tmp_cn, *this);
  }

} // end of communication net (cn) namespace

