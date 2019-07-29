
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
#include "morpheus/Formats/Formatter.hpp"

#include <memory>
#include <string>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

namespace cn {
using namespace llvm;

enum EdgeType {
  SINGLE_HEADED,
  // just for input edges
  DOUBLE_HEADED,
  SINGLE_HEADED_RO,
  DOUBLE_HEADED_RO,
  SHUFFLE,
  SHUFFLE_RO,
};

enum EdgeCategory {
  REGULAR,
  CONTROL_FLOW,
};

template <typename T>
struct Printable {
  virtual ~Printable() = default;

  virtual void print(ostream &os, const formats::Formatter &fmt) const {
    fmt.format(os, static_cast<const T &>(*this));
  }
};


struct Identifiable {
  using ID = string;

  virtual ~Identifiable() = default;

  Identifiable() : id(generate_id()) { }
  Identifiable(const Identifiable &) = delete;
  Identifiable(Identifiable &&) = default;
  Identifiable& operator=(const Identifiable &) = delete;
  Identifiable& operator=(Identifiable &&) = default;

  ID get_id() const {
    return id;
  }

private:
  static ID generate_id();

  ID id;
};


struct Edge;

struct NetElement : public Identifiable, public Printable<NetElement> {

  virtual ~NetElement() = default;

  NetElement(string name) : name(name) { }
  NetElement(const NetElement &) = delete;
  NetElement(NetElement &&) = default;
  NetElement& operator=(const NetElement &) = delete;
  NetElement& operator=(NetElement &&) = default;

  virtual string get_element_type() const = 0;

  string name;
  vector<unique_ptr<Edge>> leads_to;

  // NOTE: non-owning pointers to edges that points to the element
  vector<Edge*> referenced_by;
};


struct Edge final : public Printable<Edge> {

  explicit Edge(NetElement &startpoint, NetElement &endpoint, string arc_expr,
                EdgeCategory category, EdgeType type)
    : startpoint(startpoint), endpoint(endpoint), arc_expr(arc_expr),
      category(category), type(type) { }
  Edge(const Edge &) = delete;
  Edge(Edge &&) = default;
  Edge& operator=(const Edge &) = delete;
  Edge& operator=(Edge &&) = default;

  inline EdgeCategory get_category() const { return category; }
  inline EdgeType     get_type()     const { return type; }

  NetElement &startpoint;
  NetElement &endpoint;
  string arc_expr;

private:
  EdgeCategory category;
  EdgeType type;
};


struct Place final : NetElement {

  explicit Place(string name, string type, string init_expr)
    : NetElement(name), type(type), init_expr(init_expr) { }
  Place(const Place &) = delete;
  Place(Place &&) = default;
  Place& operator=(const Place &) = delete;
  Place& operator=(Place &&) = default;

  string get_element_type() const {
    return "place_t";
  }

  string type;
  string init_expr;
};


using ConditionList = vector<string>;

struct Transition final : NetElement {

  explicit Transition(string name, const ConditionList guard)
    : NetElement(name), guard(guard) { }
  Transition(const Transition &) = delete;
  Transition(Transition &&) = default;
  Transition& operator=(const Transition &) = delete;
  Transition& operator=(Transition &&) = default;

  string get_element_type() const {
    return "transition_t";
  }

  string name;
  ConditionList guard;
};


// ==========================================================
// CommunicationNet

class CommunicationNet : public Identifiable,
                         public Printable<CommunicationNet> {

  template <typename T>
  using Element = unique_ptr<T>;

  template <typename T>
  using Elements = vector<Element<T>>;

  template <typename T, typename... Args>
  Element<T> make_element_(Args&&... args) {
    return std::make_unique<T>(forward<Args>(args)...);
  }

public:
  // EdgePredicate is used to filter a category of edges
  template <EdgeCategory C>
  struct EdgePredicate {
    bool operator()(const Edge &e) const {
      return e.get_category() == C;
    }
  };

protected:

  void remove_refs(const NetElement &elem) {
    for (const std::unique_ptr<Edge> &edge : elem.leads_to) {
      NetElement &pointed_elem = edge->endpoint;
      auto new_end = std::remove(pointed_elem.referenced_by.begin(),
                                 pointed_elem.referenced_by.end(),
                                 edge.get());

      auto old_end = pointed_elem.referenced_by.end();
      if (new_end != old_end) { // remove erase unspecified elements
        pointed_elem.referenced_by.erase(new_end, old_end);
      }
    }
  }

  bool is_collapsible(const Edge &e) const {
    EdgePredicate<CONTROL_FLOW> is_cf;

    if (e.startpoint.get_element_type() == e.endpoint.get_element_type() && is_cf(e)) {
      if (e.startpoint.get_element_type() == "place_t") {
        Place &p1 = static_cast<Place&>(e.startpoint);
        Place &p2 = static_cast<Place&>(e.endpoint);
        if (p1.type == p2.type) {
          return true;
        }
      } else {
        return true;
      }
    }
    return false;
  }

  void reconnect(const Edge &e) {
    NetElement &startpoint = e.startpoint;
    for (Edge *ref_e : startpoint.referenced_by) {
      Elements<Edge> &owner_storage = ref_e->startpoint.leads_to;

      // find a unique pointer owning the reference pointer `ref_e`
      auto owner_it = std::find_if(owner_storage.begin(),
                             owner_storage.end(),
                             [ref_e](const Element<Edge> &o) { return o.get() == ref_e; });

      assert (owner_it != owner_storage.end() && "The owner has to exist!");

      Element<Edge> &edge = *owner_it;

      // create a new bypassing edge
      Element<Edge> new_edge = create_edge_(edge->startpoint, e.endpoint, edge->arc_expr,
                                            edge->get_category(), edge->get_type());

      // swap the new edge with the old one
      std::swap(edge, new_edge);
    }
  }

  template <typename T>
  void collapse(Elements<T> &elements,
                CommunicationNet &tmp_cn,
                Elements<T> CommunicationNet::*storage) {
    size_t idx = 0;
    while (idx < elements.size()) {
      Element<T> &elem = elements[idx];
      if (elem->leads_to.size() == 1) { // only one-path nodes can be collapsed
        Element<Edge> &e = elem->leads_to.back();
        if (is_collapsible(*e)) {
          remove_refs(*elem); // remove the references stored within `referenced_by`
                              // info at net elements
          reconnect(*e);

          // skip the element, hence remove it
          idx++;
          continue;
        }
      }
      // overtake element
      tmp_cn.add_(move(elem), tmp_cn.*storage);
      idx++;
    }
  }

public:
  virtual ~CommunicationNet() = default;

  CommunicationNet() = default;
  CommunicationNet(const CommunicationNet &) = delete;
  CommunicationNet(CommunicationNet &&) = default;
  CommunicationNet& operator=(const CommunicationNet &) = delete;
  CommunicationNet& operator=(CommunicationNet &&) = default;

  virtual void collapse();
  virtual void takeover(CommunicationNet cn);

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

  Edge& add_input_edge(Place &src, Transition &dest, string ae="",
                       EdgeType type=SINGLE_HEADED) {
    return add_edge_(src, dest, ae, REGULAR, type);
  }

  Edge& add_output_edge(Transition &src, Place &dest, string ae="") {
    return add_edge_(src, dest, ae, REGULAR, SINGLE_HEADED);
  }

  template<typename Startpoint, typename Endpoint>
  Edge& add_cf_edge(Startpoint& src, Endpoint& dest) {
    return add_edge_(src, dest, "", CONTROL_FLOW, SINGLE_HEADED);
  }

  // -------------------------------------------------------
  // iterators

  iterator_range<typename Elements<Place>::iterator> places() {
    return make_range(places_.begin(), places_.end());
  }

  iterator_range<typename Elements<Place>::const_iterator> places() const {
    return make_range(places_.begin(), places_.end());
  }

  iterator_range<typename Elements<Transition>::iterator> transitions() {
    return make_range(transitions_.begin(), transitions_.end());
  }

  iterator_range<typename Elements<Transition>::const_iterator> transitions() const {
    return make_range(transitions_.begin(), transitions_.end());
  }

private:
  template <typename T>
  inline T& add_(Element<T> &&e, Elements<T> &elements) {
    elements.push_back(forward<Element<T>>(e));
    return *elements.back();
  }

  template <typename Startpoint, typename Endpoint>
  inline Element<Edge> create_edge_(Startpoint &start, Endpoint &end, string ae,
                                    EdgeCategory category, EdgeType type) {

    Element<Edge> edge = make_element_<Edge>(start, end, ae, category, type);
    // the end element keeps the pointer to edge which is pointing to it
    end.referenced_by.push_back(edge.get());

    return edge;
  }

  inline Edge& add_edge_(Element<Edge> edge) {
    return add_(move(edge), edge->startpoint.leads_to);
  }

  template <typename Startpoint, typename Endpoint>
  inline Edge& add_edge_(Startpoint &start, Endpoint &end, string ae,
                  EdgeCategory category, EdgeType type) {

    return add_edge_(create_edge_(start, end, ae, category, type));
  }

  template <typename T>
  inline void takeover_(Elements<T> &target, iterator_range<typename Elements<T>::iterator> src) {
    move(src.begin(), src.end(), back_inserter(target));
  }

  Elements<Place> places_;
  Elements<Transition> transitions_;
};

// =============================================================================
// Addressable CN

struct AddressableCN final : public CommunicationNet {

  using Address = unsigned int;

  const Address address;
  Place &asr;
  Place &arr;
  Place &csr;
  Place &crr;
  // inner communication net used to keep away ACN elements from those of plugged nets
  CommunicationNet embedded_cn;

  ~AddressableCN() = default;

  AddressableCN(Address address)
    : address(address),
      asr(add_place("MessageToken", "", "ActiveSendRequest")),
      arr(add_place("MessageRequest", "", "ActiveReceiveRequest")),
      csr(add_place("MessageRequest", "", "CompletedSendRequest")),
      crr(add_place("MessageToken", "", "CompletedReceiveRequest")),
      embedded_cn(CommunicationNet()),
      entry_p_(&add_place("Unit", "", "ACN" + std::to_string(address) + "Entry" + get_id())),
      exit_p_(&add_place("Unit", "", "ACN" + std::to_string(address) + "Exit" + get_id())) { }

  AddressableCN(const AddressableCN &) = delete;
  AddressableCN(AddressableCN &&) = default;

  // NOTE: the print is redefined to force the formatter to use the right method
  void print(ostream &os, const formats::Formatter &fmt) const {
    fmt.format(os, *this);
  }

  void collapse () {
    embedded_cn.collapse();
  }

  void takeover (CommunicationNet cn) {
    embedded_cn.takeover(move(cn));
  }

  // ---------------------------------------------------------------------------
  // AddressableCN follows the interface of PluginCN in the sense that it has
  // also entry and exit places. But cannot be injected!

  Place& entry_place() { return *entry_p_; }
  Place& exit_place() { return *exit_p_; }

  void set_entry(Place &p) { entry_p_ = &p; }
  void set_exit(Place &p) { exit_p_ = &p; }

private:
  Place *entry_p_;
  Place *exit_p_;
};


// =============================================================================
// Generic Plugin CN

class PluginCNGeneric {

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

  void add_cf_edge(NetElement& src, NetElement& dest) {
    self_->add_cf_edge_(src, dest);
  }

  void takeover(CommunicationNet cn) { self_->takeover_(move(cn)); }

  Place& entry_place() { return self_->entry_place_(); }
  Place& exit_place() { return self_->exit_place_(); }

  void set_entry(Place &p) { self_->set_entry_(p); }
  void set_exit(Place &p) { self_->set_exit_(p); }

  void print(ostream &os, const formats::Formatter &fmt) const {
    self_->print_(os, fmt);
  }

  // ---------------------------------------------------------------------------
  // interface and model implementation of pluggable types

private:
  struct pluggable_t { // interface of pluggable CNs
    virtual ~pluggable_t() = default;

    virtual void takeover_(CommunicationNet cn) = 0;
    virtual void inject_into_(AddressableCN &) = 0;
    virtual void inject_into_(PluginCNGeneric &) = 0;
    virtual void add_cf_edge_(NetElement &src, NetElement &dest) = 0;
    virtual Place& entry_place_() = 0;
    virtual Place& exit_place_() = 0;
    virtual void set_entry_(Place &) = 0;
    virtual void set_exit_(Place &) = 0;
    virtual void print_(ostream &os, const formats::Formatter &fmt) const = 0;
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

    void add_cf_edge_(NetElement &src, NetElement &dest) override {
      pcn_.add_cf_edge(src, dest);
    }

    Place& entry_place_() override {
      return pcn_.entry_place();
    }

    Place& exit_place_() override {
      return pcn_.exit_place();
    }

    void set_entry_(Place &p) override {
      pcn_.set_entry(p);
    }

    void set_exit_(Place &p) override {
      pcn_.set_exit(p);
    }

    void print_(ostream &os, const formats::Formatter &fmt) const override {
      pcn_.print(os, fmt);
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
    : entry_p_(&add_place("Unit", "", "entry" + get_id())),
      exit_p_(&add_place("Unit", "", "exit" + get_id())) { }
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

  Place& entry_place() { return *entry_p_; }
  Place& exit_place() { return *exit_p_; }

  void set_entry(Place &p) { entry_p_ = &p; }
  void set_exit(Place &p) { exit_p_ = &p; }

private:
  template <typename PluggableCN>
  void plug_in_(PluggableCN &pcn) {
    // join entry & exit places
    pcn.add_cf_edge(pcn.entry_place(), entry_place());

    // set the new entry as the exit of injected net
    pcn.set_entry(exit_place());

    // pass over the elements into `pcn`
    pcn.takeover(move(*this));
  }

  Place *entry_p_;
  Place *exit_p_;
};

} // end of anonymous namespace

#endif // MRPH_COMM_NET_H
