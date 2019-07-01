
#ifndef COMM_NET_FACTORY_H
#define COMM_NET_FACTORY_H

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"

#include "morpheus/Utils.hpp"
#include "morpheus/ADT/CommunicationNet.hpp"

#include <functional>

namespace {
using namespace llvm;

class EmptyCommNet : public PluginCommNet {

public:
  EmptyCommNet() {
    add_cf_edge(entry_place(), exit_place());
  }
  EmptyCommNet(const EmptyCommNet &) = delete;
  EmptyCommNet(EmptyCommNet &&) = default;
};

class BaseSendRecv : public PluginCommNet {

protected:

  // partial elements
  std::string msg_rqst;

  // full elements
  std::string setting_type;
  std::string from_setting_ae;
  std::string data_type;
  std::string from_data_ae;

  void compute_setting_info(Value const *src,
                            Value const *dest,
                            Value const *tag,
                            std::string buffered) {

    using namespace std::placeholders;
    auto store_non_empty = std::bind(store_if_not<std::string>, _1, _2, "");

    std::vector<std::string> types, names;
    std::string src_val, dest_val, tag_val;
    // determine str, dest, val if not null
    if (src) {
      store_non_empty(types, value_to_type(*src, false));
      store_non_empty(names, value_to_str(*src, "src", false));
      src_val = value_to_str(*src, "src");
    }
    if (dest) {
      store_non_empty(types, value_to_type(*dest, false));
      store_non_empty(names, value_to_str(*dest, "dest", false));
      dest_val = value_to_str(*dest, "dest");
    }
    assert (tag != nullptr && "Expected not null tag value.");
    store_non_empty(types, value_to_type(*tag, false));
    store_non_empty(names, value_to_str(*tag, "tag", false));
    tag_val = value_to_str(*tag, "tag");

    setting_type = pp_vector(types, ", ", "(", ")");
    from_setting_ae = pp_vector(names, ", ", "(", ")");

    msg_rqst = generate_message_request(src_val, dest_val, tag_val, buffered);
  }

  void compute_data_buffer_info(const Value &type) {
    data_type = "DataPacket";
    from_data_ae = "data";
  }

};

class CN_MPI_Isend : public BaseSendRecv {

  // MPI_Isend(
  //   const void* buf,        // data set to 'send_data' -- this is done within the corresponding annotation
  //   int count,              // information on the input-arc from send_data -- where I can find it?
  //   MPI_Datatype datatype,  // data type of 'send_data' place
  //   int dest,               // it goes to the setting place
  //   int tag,                // it goes to the setting place
  //   MPI_Comm comm           // THIS IS IGNORED AND IT IS SUPPOSED TO BE MPI_COMM_WORLD
  // );

  std::string name_prefix;
  Place &send_setting; // INPUT
  Place &send_data;    // INPUT
  Place &send_reqst;   // OUTPUT
  Place &send_exit;    // unit exit place
  Transition &send;

public:
  CN_MPI_Isend(const CallSite &cs)
    : name_prefix("send" + std::to_string(get_id())),
      send_setting(add_place("<empty>", "", name_prefix + "_setting")),
      send_data(add_place("<empty>", "", name_prefix + "_data")),
      send_reqst(add_place("(MPI_Request, MessageRequest)", "", name_prefix + "_reqst")),
      send_exit(add_place("Unit", "", name_prefix + "_exit")),
      send(add_transition({}, name_prefix)) {

    Value *datatype = cs.getArgument(2);
    compute_data_buffer_info(*datatype);
    send_data.type = data_type;

    Value *dest = cs.getArgument(3);
    Value *tag = cs.getArgument(4);
    compute_setting_info(nullptr, dest, tag, "buffered");
    send_setting.type = setting_type;

    add_input_edge(send_data, send, TAKE, from_data_ae);
    add_input_edge(send_setting, send, TAKE, from_setting_ae);

    add_cf_edge(entry_place(), send_setting);
    add_cf_edge(send_exit, exit_place());
  }

  virtual void connect_asr(const Place &p) {
    add_output_edge(send, p);
  }
};

class CN_MPI_Wait : public BaseSendRecv {

  // MPI_Wait(
  //   MPI_Request *request // INOUT
  //   MPI_Status *status   // OUT
  // )

  std::string name_prefix;
  Transition &wait;

public:
  CN_MPI_Wait(const CallSite &cs)
    : name_prefix("wait" + std::to_string(get_id())),
      wait(add_transition({}, name_prefix)) { }

  virtual void connect_csr(const Place &p) {
    // TODO: add input arcs according to the request type
  }

  virtual void connect_crr(const Place &p) {
    // TODO: add input arcs according to the request type
  }
};

struct CommNetFactory {

  static std::unique_ptr<PluginCommNet> createCommNet(const CallSite &cs) {
    Function *f = cs.getCalledFunction();
    assert (f->hasName() && "The CommNetFactory expects call site with a named function");

    StringRef call_name = f->getName();
    if (call_name == "MPI_Isend" || call_name == "MPI_Send" /* TODO: remove MPI_Send */) {
      return std::make_unique<CN_MPI_Isend>(cs);
    }
    return std::make_unique<EmptyCommNet>();
  }
};

} // end of anonymous namespace
#endif // COMM_NET_FACTORY_H
