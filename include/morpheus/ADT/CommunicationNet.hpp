
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

  virtual void print (raw_ostream &os) const {
    os << "CommunicationNet(" << id << "):\n";

    os << "Places:\n";
    print_(places_, os, 2);

    os << "Transitions:\n";
    print_(transitions_, os, 2);

    os << "Input edges:\n";
    print_(input_edges_, os, 2);

    os << "Outuput edges:\n";
    print_(output_edges_, os, 2);

    os << "CF edges:\n";
    print_(cf_edges_, os, 2);
  }

  // -------------------------------------------------------
  // iterators

  iterator_range<places_iterator> places() {
    return make_range(places_.begin(), places_.end());
  }

  iterator_range<const_places_iterator> places() const {
    return make_range(places_.begin(), places_.end());
  }

  iterator_range<transitions_iterator> transitions() {
    return make_range(transitions_.begin(), transitions_.end());
  }

  iterator_range<const_transitions_iterator> transitions() const {
    return make_range(transitions_.begin(), transitions_.end());
  }

  iterator_range<edges_iterator> input_edges() {
    return make_range(input_edges_.begin(), input_edges_.end());
  }

  iterator_range<const_edges_iterator> input_edges() const {
    return make_range(input_edges_.begin(), input_edges_.end());
  }

  iterator_range<edges_iterator> output_edges() {
    return make_range(output_edges_.begin(), output_edges_.end());
  }

  iterator_range<const_edges_iterator> output_edges() const {
    return make_range(output_edges_.begin(), output_edges_.end());
  }

  iterator_range<edges_iterator> control_flow_edges() {
    return make_range(cf_edges_.begin(), cf_edges_.end());
  }

  iterator_range<const_edges_iterator> control_flow_edges() const {
    return make_range(cf_edges_.begin(), cf_edges_.end());
  }

private:
  template <typename T, typename... Args>
  Element<T> make_element_(Args&&... args) {
    return std::make_unique<T>(forward<Args>(args)...);
  }

  template <typename T>
  T& add_(Element<T> e, Elements<T> &elements) {
    elements.push_back(move(e));
    return *elements.back();
  }

  template <typename T>
  void takeover_(Elements<T> &target, iterator_range<typename Elements<T>::iterator> src) {
    move(src.begin(), src.end(), back_inserter(target));
  }

  template <typename T>
  void print_(const Elements<T> &elements, raw_ostream &os, size_t pos=0) const {
    for (const auto &e : elements) {
      os << std::string(pos, ' ');
      print_(e, os);
      os << "\n";
    }
  }

  template <typename T>
  void print_(const Element<T> &elem, raw_ostream &os, size_t pos=0) const {
    os << std::string(pos, ' ') << *elem; // Element<T> = unique_ptr<T>
  }

  Elements<Place> places_;
  Elements<Transition> transitions_;

  Elements<Edge> input_edges_;
  Elements<Edge> output_edges_;
  Elements<Edge> cf_edges_;
};

// =============================================================================
// Addressable CN

struct AddressableCN final : public CommunicationNet {

  using Address = unsigned int;

  const Address address;
  const Place &asr;
  const Place &arr;
  const Place &csr;
  const Place &crr;

  ~AddressableCN() = default;

  AddressableCN(Address address)
    : address(address),
      asr(add_place("MessageToken", "", "Active Send Request")),
      arr(add_place("MessageRequest", "", "Active Receive Request")),
      csr(add_place("MessageRequest", "", "Completed Send Request")),
      crr(add_place("MessageToken", "", "Completed Receive Request")),
      entry_p_(&add_place("Unit", "", "Entry" + id)),
      exit_p_(&add_place("Unit", "", "Exit" + id)) { }

  AddressableCN(const AddressableCN &) = delete;
  AddressableCN(AddressableCN &&) = default;

  void print (raw_ostream &os) const {
    os << "Adress: " << address << "\n";
    os << "------------------------------------------------------------\n";
    CommunicationNet::print(os);
    os << "------------------------------------------------------------\n\n";
  }

  // ---------------------------------------------------------------------------
  // AddressableCN follows the interface of PluginCN in the sense that it has
  // also entry and exit places. But cannot be injected!

  Place& entry_place() {
    return *entry_p_;
  }

  Place& exit_place() {
    return *exit_p_;
  }

  void set_entry(Place *p) {
    entry_p_ = p;
  }

  void set_exit(Place *p) {
    exit_p_ = p;
  }

private:
  Place *entry_p_;
  Place *exit_p_;
};


// =============================================================================
// Generic Plugin CN

class PluginCNGeneric final : public Printable {

public:
  ~PluginCNGeneric() = default;

  template <typename PluggableCN>
  PluginCNGeneric(PluggableCN &&net)
    : self_(std::make_unique<model<PluggableCN>>(forward<PluggableCN>(net))) { }

  PluginCNGeneric(const PluginCNGeneric &) = delete;
  PluginCNGeneric(PluginCNGeneric &&) = default;

  // ---------------------------------------------------------------------------
  // accessible methods of PluggableCNs

  template <typename PluggableCN>
  void inject_into(PluggableCN &pcn) {
    move(self_)->inject_into_(pcn);
    self_.release();
  }

  void add_cf_edge(const NetElement& src, const NetElement& dest) {
    self_->add_cf_edge_(src, dest);
  }

  void takeover(CommunicationNet cn) {
    self_->takeover_(move(cn));
  }

  Place& entry_place() {
    return self_->entry_place_();
  }

  Place& exit_place() {
    return self_->exit_place_();
  }

  void set_entry(Place *p) {
    self_->set_entry_(p);
  }

  void set_exit(Place *p) {
    self_->set_exit_(p);
  }

  void print(raw_ostream &os) const {
    self_->print_(os);
  }

private:
  struct pluggable_t { // interface of pluggable CNs
    virtual ~pluggable_t() = default;

    virtual void takeover_(CommunicationNet cn) = 0;
    virtual void inject_into_(AddressableCN &) = 0;
    virtual void inject_into_(PluginCNGeneric &) = 0;
    virtual void add_cf_edge_(const NetElement &src, const NetElement &dest) = 0;
    virtual Place& entry_place_() = 0;
    virtual Place& exit_place_() = 0;
    virtual void set_entry_(Place *) = 0;
    virtual void set_exit_(Place *) = 0;
    virtual void print_(raw_ostream &) const = 0;
  };

  template <typename PluggableCN>
  struct model final : pluggable_t {
    model(PluggableCN &&pcn) : pcn_(forward<PluggableCN>(pcn)) { }

    void takeover_(CommunicationNet cn) override {
      pcn_.takeover(move(cn));
    }

    void inject_into_(AddressableCN &acn) override {
      move(pcn_).inject_into(acn);
    }

    void inject_into_(PluginCNGeneric &pcn) override {
      move(pcn_).inject_into(pcn);
    }

    void add_cf_edge_(const NetElement &src, const NetElement &dest) override {
      pcn_.add_cf_edge(src, dest);
    }

    Place& entry_place_() override {
      return pcn_.entry_place();
    }

    Place& exit_place_() override {
      return pcn_.exit_place();
    }

    void set_entry_(Place *p) override {
      pcn_.set_entry(p);
    }

    void set_exit_(Place *p) override {
      pcn_.set_exit(p);
    }

    void print_(raw_ostream &os) const override {
      pcn_.print(os);
    }

    PluggableCN pcn_;
  };

  unique_ptr<pluggable_t> self_;
};


// =============================================================================
// Base Plugin CN

class PluginCNBase : public CommunicationNet {
public:
  virtual ~PluginCNBase() = default;

  PluginCNBase()
    : entry_p_(&add_place("Unit", "", "entry" + id)),
      exit_p_(&add_place("Unit", "", "exit" + id)) { }
  PluginCNBase(const PluginCNBase &) = delete;
  PluginCNBase(PluginCNBase &&) = default;

  virtual void connect(const AddressableCN &acn) = 0;

  template <typename PluggableCN>
  void inject_into(PluggableCN &) & {
    assert(false && "Only an r-value reference can be injected into a PluggableCN\n");
  }

  void inject_into(AddressableCN &acn) && {
    connect(acn);
    plug_in_(acn);
  }

  void inject_into(PluginCNGeneric &pcn) && {
    plug_in_(pcn);
  }

  Place& entry_place() {
    return *entry_p_;
  }

  Place& exit_place() {
    return *exit_p_;
  }

  void set_entry(Place *p) {
    entry_p_ = p;
  }

  void set_exit(Place *p) {
    exit_p_ = p;
  }

private:
  template <typename PluggableCN>
  void plug_in_(PluggableCN &pcn) {
    // join entry & exit places
    pcn.add_cf_edge(pcn.entry_place(), entry_place());

    // set the new entry as the exit of injected net
    pcn.set_entry(&exit_place());

    // pass over the elements into `pcn`
    pcn.takeover(move(*this));
  }

  Place *entry_p_;
  Place *exit_p_;
};

} // end of anonymous namespace

#endif // MRPH_COMM_NET_H
