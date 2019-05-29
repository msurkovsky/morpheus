
//===----------------------------------------------------------------------===//
//
// CommunicationNet
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_COMM_NET_H
#define MRPH_COMM_NET_H

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

class CommunicationNet {
public:
  enum EdgeType {
    TAKE,
    // just for input edges
    FORCE_TAKE,
    READ_ONLY,
    FORCE_READY_ONLY,
    SHUFFLE,
  };

private:
  using ConditionList = vector<string>;

  struct IEdge { };
  template<class SrcT, class DestT>
  struct EdgeT : IEdge {
    SrcT src;
    DestT dest;
    EdgeType type;

    explicit EdgeT(SrcT src, DestT dest, EdgeType type)
      : src(src), dest(dest), type(type) { }
    EdgeT(const EdgeT &) = delete;
    EdgeT(EdgeT &&) = default;
  };

public:
  struct PlaceT {
    string name;
    string type;
    string init_expr;

    explicit PlaceT(string name, string type, string init_expr)
      : name(name), type(type), init_expr(init_expr) { }
  };

  struct TransitionT {
    string name;
    const ConditionList &conditions;

    explicit TransitionT(string name, const ConditionList &cl)
      : name(name), conditions(cl) { }
  };

  using PlaceRef = unique_ptr<PlaceT>&;
  using TransitionRef = unique_ptr<TransitionT>&;

  using InputEdgeT = EdgeT<PlaceRef, TransitionRef>;
  using OutputEdgeT = EdgeT<TransitionRef, PlaceRef>;

private:
  vector<unique_ptr<PlaceT>> places;
  vector<unique_ptr<TransitionT>> transitions;

  vector<unique_ptr<InputEdgeT>> input_edges;
  vector<unique_ptr<OutputEdgeT>> output_edges;
  vector<unique_ptr<IEdge>> control_flow_edges;

public:
  PlaceRef add_place(string type, string init_expr, string name="") {
    places.push_back(make_unique<PlaceT>(name, type, init_expr));
    return places.back();
  }

  TransitionRef add_transition(ConditionList &cl, string name="") {
    transitions.push_back(make_unique<TransitionT>(name, cl));
    return transitions.back();
  }

  void add_input_edge(PlaceRef src, TransitionRef dest, EdgeType type=TAKE) {
    input_edges.push_back(make_unique<InputEdgeT>(src, dest, type));
  }

  void add_output_edge(TransitionRef src, PlaceRef dest) {
    output_edges.push_back(make_unique<OutputEdgeT>(src, dest, TAKE));
  }

  template<class SrcT, class DestT>
  void add_cf_edge(SrcT src, DestT dest) {
    control_flow_edges.push_back(make_unique<EdgeT<SrcT, DestT>>(src, dest, TAKE));
  }
};

#endif // MRPH_COMM_NET_H
