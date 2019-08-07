
//===----------------------------------------------------------------------===//
//
// CommunicationNet
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_COMM_NET_H
#define MRPH_COMM_NET_H

#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/raw_ostream.h"

#include "morpheus/Utils.hpp"

#include <memory>
#include <string>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

namespace {
using namespace llvm;

enum EdgeType {
  TAKE,
  // just for input edges
  FORCE_TAKE,
  READ_ONLY,
  FORCE_READY_ONLY,
  SHUFFLE,
};

struct Printable {
  virtual ~Printable() = default;

  virtual void print (raw_ostream &os) const = 0;
};

raw_ostream &operator<< (raw_ostream &os, const Printable &printable) {
  printable.print(os);
  return os;
}


struct Identifiable {
  using ID = string;

  const ID id;

  Identifiable() : id(generate_id()) { }

  virtual ~Identifiable() = default;

private:
  static ID generate_id() {
    static unsigned int id = 0;
    return std::to_string(++id);
  }
};


struct NetElement : public Identifiable, public Printable {
  string name;

  virtual ~NetElement() = default;

  NetElement(string name) : name(name) { }
  NetElement(const NetElement &) = delete;
  NetElement(NetElement &&) = default;

  virtual void print (raw_ostream &os) const {
    if (name.empty()) {
      os << this; // print pointer value
    } else {
      os << name;
    }
  }
};


struct Edge final : public Printable {
  const NetElement &startpoint;
  const NetElement &endpoint;
  EdgeType type; // TODO: maybe define separates types for it
  string arc_expr;

  explicit Edge(const NetElement &startpoint, const NetElement &endpoint,
                EdgeType type, string arc_expr)
    : startpoint(startpoint), endpoint(endpoint),
      type(type), arc_expr(arc_expr) { }
  Edge(const Edge &) = delete;
  Edge(Edge &&) = default;

  void print (raw_ostream &os) const {
    if (arc_expr.empty()) {
      os << startpoint << " -> " << endpoint;
    } else {
      os << startpoint << " --/ " << arc_expr << " /--> " << endpoint;
    }
  }
};


struct Place final : NetElement {
  string type;
  string init_expr;

  explicit Place(string name, string type, string init_expr)
    : NetElement(name), type(type), init_expr(init_expr) { }
  Place(const Place &) = delete;
  Place(Place &&) = default;

  virtual void print (raw_ostream &os) const {
    os << "P(" << id << "): ";

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
};


using ConditionList = vector<string>;

struct Transition final : NetElement {
  string name;
  const ConditionList guard;

  explicit Transition(string name, const ConditionList guard)
    : NetElement(name), guard(guard) { }
  Transition(const Transition &) = delete;
  Transition(Transition &&) = default;

  virtual void print (raw_ostream &os) const {
    os << "T(" << id << "): ";

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
};


// ==========================================================
// CommunicationNet

class CommunicationNet : public Identifiable,
                         public Printable {

  template <typename T>
  using Element = unique_ptr<T>;

  template <typename T>
  using Elements = vector<Element<T>>;

public:
  using places_iterator      = Elements<Place>::iterator;
  using transitions_iterator = Elements<Transition>::iterator;
  using edges_iterator       = Elements<Edge>::iterator;

  using const_places_iterator      = Elements<Place>::const_iterator;
  using const_transitions_iterator = Elements<Transition>::const_iterator;
  using const_edges_iterator       = Elements<Edge>::const_iterator;

  virtual ~CommunicationNet() = default;

  CommunicationNet() = default;
  CommunicationNet(const CommunicationNet &) = delete;
  CommunicationNet(CommunicationNet &&) = default;

  Place& add_place(string type, string init_expr, string name="") {
    return add_(make_element_<Place>(name, type, init_expr), places_);
  }

  Place& add_place(Element<Place> p) {
    return add_(move(p), places_);
  }

  Transition& add_transition(ConditionList cl, string name="") {
    return add_(make_element_<Transition>(name, cl), transitions_);
  }

  Transition& add_transition(Element<Transition> t) {
    return add_(move(t), transitions_);
  }

  Edge& add_input_edge(const Place &src, const Transition &dest, EdgeType type=TAKE, string ae="") {
    return add_(make_element_<Edge>(src, dest, type, ae), input_edges_);
  }

  Edge& add_input_edge(unique_ptr<Edge> e) {
    return add_(move(e), input_edges_);
  }

  Edge& add_output_edge(const Transition &src, const Place &dest, string ae="") {
    return add_(make_element_<Edge>(src, dest, TAKE, ae), output_edges_);
  }

  Edge& add_output_edge(unique_ptr<Edge> e) {
    return add_(move(e), output_edges_);
  }

  template<typename Startpoint, typename Endpoint>
  Edge& add_cf_edge(const Startpoint& src, const Endpoint& dest, string ae="") {
    return add_(make_element_<Edge>(src, dest, TAKE, ae), cf_edges_);
  }

  Edge &add_cf_edge(unique_ptr<Edge> e) {
    return add_(move(e), cf_edges_);
  }

  void takeover(CommunicationNet cn) {
    takeover_(places_, cn.places());
    takeover_(transitions_, cn.transitions());
    takeover_(input_edges_, cn.input_edges());
    takeover_(output_edges_, cn.output_edges());
    takeover_(cf_edges_, cn.control_flow_edges());
  }

  }

  places_iterator       places_begin()       { return place_list.begin(); }
  const_places_iterator places_begin() const { return place_list.begin(); }
  places_iterator       places_end()         { return place_list.end(); }
  const_places_iterator places_end()   const { return place_list.end(); }
  bool                  places_empty() const { return place_list.empty(); }

  iterator_range<places_iterator> places() {
    return make_range(places_begin(), places_end());
  }

  iterator_range<const_places_iterator> places() const {
    return make_range(places_begin(), places_end());
  }


  transitions_iterator       transitions_begin()       { return transition_list.begin(); }
  const_transitions_iterator transitions_begin() const { return transition_list.begin(); }
  transitions_iterator       transitions_end()         { return transition_list.end(); }
  const_transitions_iterator transitions_end()   const { return transition_list.end(); }
  bool                       transitions_empty() const { return transition_list.empty(); }

  iterator_range<transitions_iterator> transitions() {
    return make_range(transitions_begin(), transitions_end());
  }

  iterator_range<const_transitions_iterator> transitions() const {
    return make_range(transitions_begin(), transitions_end());
  }


  edges_iterator       iedges_begin()       { return input_edge_list.begin(); }
  const_edges_iterator iedges_begin() const { return input_edge_list.begin(); }
  edges_iterator       iedges_end()         { return input_edge_list.end(); }
  const_edges_iterator iedges_end()   const { return input_edge_list.end(); }
  bool                 iedges_empty() const { return input_edge_list.empty(); }

  iterator_range<edges_iterator> input_edges() {
    return make_range(iedges_begin(), iedges_end());
  }

  iterator_range<const_edges_iterator> input_edges() const {
    return make_range(iedges_begin(), iedges_end());
  }


  edges_iterator       oedges_begin()       { return output_edge_list.begin(); }
  const_edges_iterator oedges_begin() const { return output_edge_list.begin(); }
  edges_iterator       oedges_end()         { return output_edge_list.end(); }
  const_edges_iterator oedges_end()   const { return output_edge_list.end(); }
  bool                 oedges_empty() const { return output_edge_list.empty(); }

  iterator_range<edges_iterator> output_edges() {
    return make_range(oedges_begin(), oedges_end());
  }

  iterator_range<const_edges_iterator> output_edges() const {
    return make_range(oedges_begin(), oedges_end());
  }


  edges_iterator       cfedges_begin()       { return control_flow_edge_list.begin(); }
  const_edges_iterator cfedges_begin() const { return control_flow_edge_list.begin(); }
  edges_iterator       cfedges_end()         { return control_flow_edge_list.end(); }
  const_edges_iterator cfedges_end()   const { return control_flow_edge_list.end(); }
  bool                 cfedges_empty() const { return control_flow_edge_list.empty(); }

  iterator_range<edges_iterator> control_flow_edges() {
    return make_range(cfedges_begin(), cfedges_end());
  }

  iterator_range<const_edges_iterator> control_flow_edges() const {
    return make_range(cfedges_begin(), cfedges_end());
  }

  virtual void print (raw_ostream &os) const {
    os << "CommunicationNet(" << id << "):\n";

    if (!places_empty()) {
      os << "Places:\n";
      for (auto &p : places()) {
        os << "  " << *p << "\n";
      }
    }

    if (!transitions_empty()) {
      os << "Transitions:\n";
      for (auto &t : transitions()) {
        os << "  " << *t << "\n";
      }
    }

    if (!iedges_empty()) {
      os << "Input edges:\n";
      for (auto &e : input_edges()) {
        os << "  " << *e << "\n";
      }
    }

    if (!oedges_empty()) {
      os << "Outuput edges:\n";
      for (auto &e : output_edges()) {
        os << "  " << *e << "\n";
      }
    }

    if (!cfedges_empty()) {
      os << "CF edges:\n";
      for (auto &e : control_flow_edges()) {
        os << "  " << *e << "\n";
      }
    }
  }
  Elements<Place> places_;
  Elements<Transition> transitions_;

  Elements<Edge> input_edges_;
  Elements<Edge> output_edges_;
  Elements<Edge> cf_edges_;
};

class PluginCommNet : public CommunicationNet {

public:
  using ID = unsigned int;

private:
  Place *entry_p;
  Place *exit_p;

public:
  PluginCommNet()
    : entry_p(&add_place("Unit", "", "entry" + id)),
      exit_p(&add_place("Unit", "", "exit" + id)) { }

  PluginCommNet(const PluginCommNet &) = delete;
  PluginCommNet(PluginCommNet &&) = default;

  Place &entry_place() {
    return *entry_p;
  };

  Place &exit_place() {
    return *exit_p;
  };

  void set_entry_place(Place *p) {
    entry_p = p;
  }

  void set_exit_place(Place *p) {
    exit_p = p;
  }

  virtual void inject_pluign_cn(std::unique_ptr<PluginCommNet> pcn) {

    // move elements
    for (auto &p : pcn->places()) {
      add_place(std::move(p));
    }

    for (auto &t : pcn->transitions()) {
      add_transition(std::move(t));
    }

    for (auto &ie : pcn->input_edges()) {
      add_input_edge(std::move(ie));
    }

    for (auto &oe : pcn->output_edges()) {
      add_output_edge(std::move(oe));
    }

    for (auto &cfe : pcn->control_flow_edges()) {
      add_cf_edge(std::move(cfe));
    }

    // join entry & exit places
    add_cf_edge(entry_place(), pcn->entry_place());

    // set the new entry as the exit of injected net
    set_entry_place(&pcn->exit_place());
  }

  virtual void print(raw_ostream &os) const {
    CommunicationNet::print(os);
  }

  // TODO: Plugin net does not know about these!
  virtual void connect_asr(const Place &asr_p) { };
  virtual void connect_arr(const Place &arr_p) { };
  virtual void connect_csr(const Place &csr_p) { };
  virtual void connect_crr(const Place &crr_p) { };



    using namespace std::placeholders;
    auto store_non_empty = std::bind(store_if_not<std::string>, _1, _2, "");

    std::vector<std::string> parts;
    parts.push_back("id=unique(id)");
    store_non_empty(parts, prepare_part("src", src));
    store_non_empty(parts, prepare_part("dest", dest));
    store_non_empty(parts, prepare_part("tag", tag));
    store_non_empty(parts, prepare_part("buffered", buffered));
    return pp_vector(parts, "," + delim, "{", "}");
  }
};

class AddressableCommNet : public PluginCommNet {

  using Address = unsigned int;

  const Address address;
  const Place &asr;
  const Place &arr;
  const Place &csr;
  const Place &crr;

public:

  AddressableCommNet(Address address)
    : address(address),
      asr(add_place("MessageToken", "", "Active Send Request")),
      arr(add_place("MessageRequest", "", "Active Receive Request")),
      csr(add_place("MessageRequest", "", "Completed Send Request")),
      crr(add_place("MessageToken", "", "Completed Receive Request")) { }

  AddressableCommNet(const AddressableCommNet &) = delete;
  AddressableCommNet(AddressableCommNet &&) = default;

  virtual void inject_pluign_cn(std::unique_ptr<PluginCommNet> pcn) {
    pcn->connect_asr(asr);
    pcn->connect_arr(arr);
    pcn->connect_csr(csr);
    pcn->connect_crr(crr);

    PluginCommNet::inject_pluign_cn(std::move(pcn));
  }

  void print (raw_ostream &os) {
    os << "Adress: " << address << "\n";
    os << "------------------------------------------------------------\n";
    CommunicationNet::print(os);
    os << "------------------------------------------------------------\n\n";
  }
};


} // end of anonymous namespace

#endif // MRPH_COMM_NET_H
