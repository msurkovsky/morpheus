
//===----------------------------------------------------------------------===//
//
// CommunicationNet
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_COMM_NET_H
#define MRPH_COMM_NET_H

#include "llvm/IR/CallSite.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
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
  vector<unique_ptr<Place>> places;
  vector<unique_ptr<Transition>> transitions;

  vector<unique_ptr<Edge>> input_edges;
  vector<unique_ptr<Edge>> output_edges;
  vector<unique_ptr<Edge>> control_flow_edges;

public:
  CommunicationNet() = default;
  CommunicationNet(const CommunicationNet &) = delete;
  CommunicationNet(CommunicationNet &&) = default;

  Place *add_place(string type, string init_expr, string name="") {
    places.push_back(std::make_unique<Place>(name, type, init_expr));
    return places.back().get();
  }

  Transition *add_transition(ConditionList cl, string name="") {
    transitions.push_back(std::make_unique<Transition>(name, cl));
    return transitions.back().get();
  }

  Edge *add_input_edge(const Place &src, const Transition &dest, EdgeType type=TAKE, std::string ae="") {
    input_edges.push_back(std::make_unique<Edge>(src, dest, type, ae));
    return input_edges.back().get();
  }

  Edge *add_output_edge(const Transition &src, const Place &dest, std::string ae="") {
    output_edges.push_back(std::make_unique<Edge>(src, dest, TAKE, ae));
    return output_edges.back().get();
  }

  template<typename Startpoint, typename Endpoint>
  Edge const *add_cf_edge(const Startpoint& src, const Endpoint& dest, std::string ae="") {
    control_flow_edges.push_back(std::make_unique<Edge>(src, dest, TAKE, ae));
    return control_flow_edges.back().get();
  }

  virtual void print (raw_ostream &os) const {
    os << "Places:\n";
    for (auto &p : places) {
      os << "  " << *p << "\n";
    }
    os << "Transitions:\n";
    for (auto &t : transitions) {
      os << "  " << *t << "\n";
    }
    os << "Input edges:\n";
    for (auto &e : input_edges) {
      os << "  " << *e << "\n";
    }
    os << "Outuput edges:\n";
    for (auto &e : output_edges) {
      os << "  " << *e << "\n";
    }
    os << "CF edges:\n";
    for (auto &e : control_flow_edges) {
      os << "  " << *e << "\n";
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

public:
  PluginCommNet() : id(generate_id()) { }
  PluginCommNet(const PluginCommNet &) = delete;
  PluginCommNet(PluginCommNet &&) = default;

  ID get_id() {
    return id;
  }
  std::string value_to_type(const Value &v) {
    if (isa<Constant>(v)) {
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
      asr(*add_place("MessageToken", "", "Active Send Request")),
      arr(*add_place("MessageRequest", "", "Active Receive Request")),
      csr(*add_place("MessageRequest", "", "Completed Send Request")),
      crr(*add_place("MessageToken", "", "Completed Receive Request")) { }

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
