
#ifndef COMM_NET_FACTORY_H
#define COMM_NET_FACTORY_H

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"

#include "morpheus/Utils.hpp"
#include "morpheus/ADT/CommunicationNet.hpp"

#include <functional>

namespace cn {
using namespace llvm;

// ------------------------------------------------------------------------------
// EmptyCN

struct EmptyCN final : public PluginCNBase {

public:
  ~EmptyCN() = default;

  EmptyCN() {
    add_cf_edge(entry_place(), exit_place());
  }
  EmptyCN(const EmptyCN &) = delete;
  EmptyCN(EmptyCN &&) = default;

  void connect(const AddressableCN &) {
    // no connection
  }

};


// ------------------------------------------------------------------------------
// CN_MPI_Isend

struct CN_MPI_Isend : public PluginCNBase {

  // MPI_Isend(
  //   const void* buf,        // data set to 'send_data' -- this is done within the corresponding annotation
  //   int count,              // information on the input-arc from send_data -- where I can find it?
  //   MPI_Datatype datatype,  // data type of 'send_data' place
  //   int dest,               // it goes to the setting place
  //   int tag,                // it goes to the setting place
  //   MPI_Comm comm           // THIS IS IGNORED AND IT IS SUPPOSED TO BE MPI_COMM_WORLD
  // );

  std::string name_prefix;
  Place &send_params;  // INPUT
  Place &send_reqst;   // OUTPUT
  Place &send_exit;    // unit exit place
  Transition &send;

  virtual ~CN_MPI_Isend() = default;

  CN_MPI_Isend(const CallSite &cs)
    : name_prefix("send" + get_id()),
      send_params(add_place("<empty>", "", name_prefix + "_params")),
      send_reqst(add_place("(MPI_Request, MessageRequest)", "", name_prefix + "_reqst")),
      send_exit(add_place("Unit", "", name_prefix + "_exit")),
      send(add_transition({}, name_prefix)) {

    size = cs.getArgument(1);
    datatype = cs.getArgument(2);
    dest = cs.getArgument(3);
    tag = cs.getArgument(4);

    send_params.type = "(" +
      compute_data_buffer_type(*datatype) + "," +
      compute_envelope_type(nullptr, dest, *tag) + ")";

    add_input_edge(send_params, send,
                   "(" + compute_data_buffer_value(*datatype, *size) + ","
                       + compute_envelope_value(nullptr, dest, *tag, false) + ")");

    add_cf_edge(entry_place(), send_params);
    add_cf_edge(send_exit, exit_place());

    // TODO: solve unresolved places
  }

  CN_MPI_Isend(const CN_MPI_Isend &) = delete;
  CN_MPI_Isend(CN_MPI_Isend &&) = default;

  virtual void connect(const AddressableCN &acn) {
    add_output_edge(send, acn.asr, "{data=" + compute_data_buffer_value(*datatype, *size)
                    + ", envelope=" + compute_msg_rqst_value(nullptr, dest, *tag, "buffered") + "}");
  }

private:
  Value const *size;
  Value const *datatype;
  Value const *dest;
  Value const *tag;
};


// ------------------------------------------------------------------------------
// CN_MPI_Isend

struct CN_MPI_Wait : public PluginCNBase {

  // MPI_Wait(
  //   MPI_Request *request // INOUT
  //   MPI_Status *status   // OUT
  // )

  std::string name_prefix;
  Transition &wait;

  virtual ~CN_MPI_Wait() = default;

  CN_MPI_Wait(/*const CallSite &cs*/)
    : name_prefix("wait" + get_id()),
      wait(add_transition({}, name_prefix)) { }
  CN_MPI_Wait(const CN_MPI_Wait &) = delete;
  CN_MPI_Wait(CN_MPI_Wait &&) = default;

  virtual void connect(const AddressableCN &acn) {
    add_input_edge(acn.csr /* TODO: choose according to type */, wait, "TODO:", SHUFFLE);
  }
};


// ===========================================================================
// CNs factory

PluginCNGeneric createCommSubnet(const CallSite &cs) {
  Function *f = cs.getCalledFunction();
  assert (f->hasName() && "The CNFactory expects call site with a named function");

  StringRef call_name = f->getName();
  if (call_name == "MPI_Isend" || call_name == "MPI_Send" /* TODO: remove MPI_Send */) {
    return CN_MPI_Isend(cs);
  }
  return EmptyCN();
}

} // end of anonymous namespace
#endif // COMM_NET_FACTORY_H
