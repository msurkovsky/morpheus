
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

struct Element : public Printable {
  using ID = unsigned int;

  const ID id;
  string name;

  Element(string name) : id(get_id()), name(name) { }
  Element(const Element &) = delete;
  Element(Element &&) = default;

  virtual void print (raw_ostream &os) const {
    if (name.empty()) {
      os << this;
    } else {
      os << name;
    }
  }

private:
  static ID get_id() {
    static ID id = 0;
    return ++id;
  }
};


struct Edge : public Printable {
  const Element &startpoint;
  const Element &endpoint;
  EdgeType type;
  std::string arc_expr;

  explicit Edge(const Element &startpoint, const Element &endpoint,
                EdgeType type, std::string arc_expr)
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


struct Place : Element {
  string type;
  string init_expr;

  explicit Place(string name, string type, string init_expr)
    : Element(name), type(type), init_expr(init_expr) { }

  Place(const Place &) = delete;
  Place(Place &&) = default;
  ~Place() = default;

  void print (raw_ostream &os) const {
    os << "P(" << id << "): ";

    Element::print(os);

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

struct Transition : Element {
  string name;
  const ConditionList guard;

  explicit Transition(string name, const ConditionList guard)
    : Element(name), guard(guard) { }

  Transition(const Transition &) = delete;
  Transition(Transition &&) = default;
  ~Transition() = default;

  void print (raw_ostream &os) const {
    os << "T(" << id << "): ";

    Element::print(os);

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


// ==============================================================================

class CommunicationNet : public Printable {
public:
  using places_iterator = vector<unique_ptr<Place>>::iterator;
  using transitions_iterator = vector<unique_ptr<Transition>>::iterator;
  using edges_iterator = vector<unique_ptr<Edge>>::iterator;

  using const_places_iterator = vector<unique_ptr<Place>>::const_iterator;
  using const_transitions_iterator = vector<unique_ptr<Transition>>::const_iterator;
  using const_edges_iterator = vector<unique_ptr<Edge>>::const_iterator;

private:
  vector<unique_ptr<Place>> place_list;
  vector<unique_ptr<Transition>> transition_list;

  vector<unique_ptr<Edge>> input_edge_list;
  vector<unique_ptr<Edge>> output_edge_list;
  vector<unique_ptr<Edge>> control_flow_edge_list;

public:
  CommunicationNet() = default;
  CommunicationNet(const CommunicationNet &) = delete;
  CommunicationNet(CommunicationNet &&) = default;

  Place &add_place(string type, string init_expr, string name="") {
    place_list.push_back(std::make_unique<Place>(name, type, init_expr));
    return *place_list.back();
  }

  Transition &add_transition(ConditionList cl, string name="") {
    transition_list.push_back(std::make_unique<Transition>(name, cl));
    return *transition_list.back();
  }

  Edge &add_input_edge(const Place &src, const Transition &dest, EdgeType type=TAKE, std::string ae="") {
    input_edge_list.push_back(std::make_unique<Edge>(src, dest, type, ae));
    return *input_edge_list.back();
  }

  Edge &add_output_edge(const Transition &src, const Place &dest, std::string ae="") {
    output_edge_list.push_back(std::make_unique<Edge>(src, dest, TAKE, ae));
    return *output_edge_list.back();
  }

  template<typename Startpoint, typename Endpoint>
  Edge const &add_cf_edge(const Startpoint& src, const Endpoint& dest, std::string ae="") {
    control_flow_edge_list.push_back(std::make_unique<Edge>(src, dest, TAKE, ae));
    return *control_flow_edge_list.back();
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
  bool                        transitions_empty() const { return transition_list.empty(); }

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
};

class PluginCommNet : public CommunicationNet {

public:
  using ID = unsigned int;

private:
  const ID id;
  static ID generate_id() {
    static ID _id = 0;
    return ++_id;
  }

  Place *entry_p;
  Place *exit_p;

public:
  PluginCommNet() : id(generate_id()) { }
  PluginCommNet(const PluginCommNet &) = delete;
  PluginCommNet(PluginCommNet &&) = default;

  virtual Place &entry_place() = 0;
  virtual Place &exit_place() = 0;

  ID get_id() {
    return id;
  }

  Place *entry_place() {
    return entry_p;
  };

  Place *exit_place() {
    return exit_p;
  };

  void set_entry_place(Place *p) {
    entry_p = p;
  }

  void set_exit_place(Place *p) {
    exit_p = p;
  }

  virtual void print(raw_ostream &os) const {
    CommunicationNet::print(os);

    }
  }

protected:
  std::string value_to_type(const Value &v, bool return_constant=true) {
    if (isa<Constant>(v) && !return_constant) {
      // NOTE: for constant value the type is represented by empty string
      //       because the constants are used directly without need to store them.
      return "";
    }
    std::string type;
    raw_string_ostream rso(type);
    rso << *v.getType();
    return rso.str();
  }

  std::string value_to_str(const Value &v, string name, bool return_constant=true) {
    if (Constant const *c = dyn_cast<Constant>(&v)) {
      if (return_constant) {
        string value;
        raw_string_ostream rso(value);
        v.printAsOperand(rso, false); // print the constant without the type
        return rso.str();
      }
      return "";
    }
    // string value;
    // raw_string_ostream rso(value);
    // v.printAsOperand(rso, false); // print used operand
    // return rso.str();
    return name;
  }

  std::string generate_message_request(std::string src,
                                       std::string dest,
                                       std::string tag,
                                       std::string buffered,
                                       std::string delim="") {

    auto prepare_part = [] (std::string name,
                            const std::string &val) -> std::string {
      if (val.empty()) {
        return "";
      }

      std::ostringstream oss;
      oss << name << "=" << val;
      return oss.str();
    };

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

class AddressableCommNet : public CommunicationNet {

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

  struct DummyT { };
  using WhereSpec = DummyT;
  using SubCN = DummyT;

  void emplace(WhereSpec wherespec, SubCN& sub_cn) {
    // connect the sub-comm. net into a place

    // TODO: Q: do I need something special from SubCN or can I use CommunicationNet itself directly?
    //       Q: what is better to enhance CN or define new "extended type?"
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
