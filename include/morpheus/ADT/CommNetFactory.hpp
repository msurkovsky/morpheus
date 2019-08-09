
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

  ~EmptyCN() = default;

  EmptyCN(const CallSite &cs) : call_name(cs.getCalledFunction()->getName()) {
    entry_place().name += call_name;
    exit_place().name += call_name;
    add_cf_edge(entry_place(), exit_place());
  }
  EmptyCN(const EmptyCN &) = delete;
  EmptyCN(EmptyCN &&) = default;

  void connect(AddressableCN &) {
    // no connection
  }

  string call_name;
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
  //   MPI_Request *request    // locally stored request that is accompanied with CN's type: MessageRequest
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

    add_output_edge(send, send_reqst,
                    "{" + compute_msg_rqst_value(nullptr, dest, *tag, "buffered") + "}");

    add_cf_edge(send, send_exit);
    add_cf_edge(entry_place(), send_params);
    add_cf_edge(send_exit, exit_place());

    // TODO: solve unresolved places
  }

  CN_MPI_Isend(const CN_MPI_Isend &) = delete;
  CN_MPI_Isend(CN_MPI_Isend &&) = default;

  virtual void connect(AddressableCN &acn) {
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
// CN_MPI_Irecv

struct CN_MPI_RecvBase : public PluginCNBase { // common base class for both blocking and non-blocking calls
  // MPI_Irecv(
  //   void* buff;             // OUT; data set to 'recv_data' -- this is done via corresponding wait
  //   int count,              // IN
  //   MPI_Datatype datatype,  // IN;  data type of 'recv_data' place
  //   int source,             // IN;  it goes to the setting place
  //   int tag,                // IN;  it goes to the setting place
  //   MPI_Comm comm,          // IN;  IGNORED (for the current version it is supposed to be MPI_COMM_WORLD)
  // - MPI_Request *request    // OUT; locally stored request it is accompanied with CNs' type: MessageRequest
  // );

  std::string name_prefix;
  Place &recv_params;
  Place &recv_data;
  Place &recv_reqst;
  Place &recv_exit;
  Transition &recv;

  virtual ~CN_MPI_RecvBase() = default;

  CN_MPI_RecvBase(const CallSite &cs):
    name_prefix("recv" + get_id()),
    recv_params(add_place("<empty>", "", name_prefix + "_params")),
    recv_data(add_place("<empty>", "", name_prefix + "_data")),
    recv_reqst(add_place("(MPI_Request, MessageRequest)", "", name_prefix + "_reqst")),
    recv_exit(add_place("Unit", "", name_prefix + "_exit")),
    recv(add_transition({}, name_prefix)) {

    size = cs.getArgument(1);
    datatype = cs.getArgument(2);
    source = cs.getArgument(3);
    tag = cs.getArgument(4);

    recv_params.type = compute_envelope_type(source, nullptr, *tag, ",", "(", ")");

    recv_data.type = compute_data_buffer_type(*datatype);

    add_input_edge(recv_params, recv,
                   compute_envelope_value(source, nullptr, *tag, false, ",", "(", ")"));

    add_output_edge(recv, recv_reqst,
                    "{" + compute_msg_rqst_value(source, nullptr, *tag, "false") + "}");

    add_cf_edge(recv, recv_exit);
    add_cf_edge(entry_place(), recv_params);
    add_cf_edge(recv_exit, exit_place());
  }

  CN_MPI_RecvBase(const CN_MPI_RecvBase &) = delete;
  CN_MPI_RecvBase(CN_MPI_RecvBase &&) = default;

  virtual void connect(AddressableCN &acn) {
    add_output_edge(recv, acn.arr, compute_msg_rqst_value(source, nullptr, *tag, "false"));
  }

protected:
  Value const *size;
  Value const *datatype;
  Value const *source;
  Value const *tag;
};

struct CN_MPI_Irecv : public CN_MPI_RecvBase {

  virtual ~CN_MPI_Irecv() = default;

  CN_MPI_Irecv(const CallSite &cs) : CN_MPI_RecvBase(cs) {
    mpi_rqst = cs.getArgument(6);

    add_unresolved_place(
      recv_reqst, *mpi_rqst,
      create_resolve_fn_(recv_data, compute_data_buffer_value(*datatype, *size)));
  }

  CN_MPI_Irecv(const CN_MPI_Irecv &) = delete;
  CN_MPI_Irecv(CN_MPI_Irecv &&) = default;

private:
  UnresolvedPlace::ResolveFnTy create_resolve_fn_(Place &recv_data, string ae_to_recv_data) {
    return [&recv_data, ae_to_recv_data](CommunicationNet &cn, Place &initiated_rqst, Transition &t_wait) {
      cn.add_input_edge(initiated_rqst, t_wait, "(reqst, {id=id})");
      cn.add_output_edge(t_wait, recv_data, ae_to_recv_data);
    };
  }

  Value const *mpi_rqst;
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

  // NOTE: An empty constructor serves to create wait without a "real" request
  // it is resolved by knowledge of particular (blocking) call.
  CN_MPI_Wait()
    : name_prefix("wait" + get_id()),
      wait(add_transition({}, name_prefix)) {

    add_cf_edge(entry_place(), wait);
    add_cf_edge(wait, exit_place());
  }

  CN_MPI_Wait(const CallSite &cs) : CN_MPI_Wait() {

    mpi_rqst = cs.getArgument(0);
    add_unresolved_transition(wait, *mpi_rqst);
  }

  CN_MPI_Wait(const CN_MPI_Wait &) = delete;
  CN_MPI_Wait(CN_MPI_Wait &&) = default;

  virtual void connect(AddressableCN &acn) {
    add_input_edge(acn.csr /* TODO: choose according to type */, wait, "TODO:", SHUFFLE);
  }

private:
  Value const *mpi_rqst;
};


// ------------------------------------------------------------------------------
// CN_MPI_Send

struct CN_MPI_Send final : public PluginCNBase {

  virtual ~CN_MPI_Send() = default;

  CN_MPI_Send(const CallSite &cs) : cn_isend(cs), cn_wait(/* TODO: */), t_wait(cn_wait.wait) {
    add_input_edge(cn_isend.send_reqst, t_wait, "(reqst, {id=id})");
    add_cf_edge(entry_place(), cn_isend.entry_place());
    add_cf_edge(cn_wait.exit_place(), exit_place());
    add_cf_edge(cn_isend.exit_place(), cn_wait.entry_place());

    takeover(std::move(cn_isend));
    takeover(std::move(cn_wait));
  }
  CN_MPI_Send(const CN_MPI_Send &) = delete;
  CN_MPI_Send(CN_MPI_Send &&) = default;


  void connect(AddressableCN &acn) {
    cn_isend.connect(acn);
    add_input_edge(acn.csr, t_wait, "[buffered] {data=data, envelope={id=id}}", SHUFFLE);
  }

private:
  CN_MPI_Isend cn_isend;
  CN_MPI_Wait cn_wait;

  Transition &t_wait;
};


// ------------------------------------------------------------------------------
// CN_MPI_Isend

struct CN_MPI_Recv final : public PluginCNBase {

  virtual ~CN_MPI_Recv() = default;

  CN_MPI_Recv(const CallSite &cs)
    : cn_irecv(cs),
      cn_wait(),
      t_wait(cn_wait.wait) {

    Value const *size = cs.getArgument(1);
    Value const *datatype = cs.getArgument(2);

    add_input_edge(cn_irecv.recv_reqst, t_wait, "(reqst, {id=id})");
    add_output_edge(t_wait, cn_irecv.recv_data,
                    compute_data_buffer_value(*datatype, *size));

    add_cf_edge(entry_place(), cn_irecv.entry_place());
    add_cf_edge(cn_wait.exit_place(), exit_place());
    add_cf_edge(cn_irecv.exit_place(), cn_wait.entry_place());

    takeover(std::move(cn_irecv));
    takeover(std::move(cn_wait));
  }

  CN_MPI_Recv(const CN_MPI_Recv &) = delete;
  CN_MPI_Recv(CN_MPI_Recv &&) = default;

  void connect (AddressableCN &acn) {
    cn_irecv.connect(acn);
    add_input_edge(acn.crr, t_wait, "{data=data, envelope={id=id}}", SHUFFLE);
  }

private:
  CN_MPI_Irecv cn_irecv;
  CN_MPI_Wait cn_wait;

  Transition &t_wait;
};

// ===========================================================================
// CNs factory

PluginCNGeneric createCommSubnet(const CallSite &cs) {
  Function *f = cs.getCalledFunction();
  assert (f->hasName() && "The CNFactory expects call site with a named function");

  StringRef call_name = f->getName();
  if (call_name == "MPI_Isend") {
    return CN_MPI_Isend(cs);
  } else if (call_name == "MPI_Send") {
    return CN_MPI_Send(cs);
  } else if (call_name == "MPI_Irecv") {
    return CN_MPI_Irecv(cs);
  } else if (call_name == "MPI_Recv") {
    return CN_MPI_Recv(cs);
  } else if (call_name == "MPI_Wait") {
    return CN_MPI_Wait(cs);
  }
  return EmptyCN(cs);
}

} // end of anonymous namespace
#endif // COMM_NET_FACTORY_H
