
#ifndef COMM_NET_FACTORY_H
#define COMM_NET_FACTORY_H

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"

#include "morpheus/Utils.hpp"
#include "morpheus/ADT/CommunicationNet.hpp"

#include <functional>
#include <sstream>

namespace {
using namespace llvm;

class BaseSendRecv : public PluginCommNet {

protected:
  std::string setting_type;
  std::string setting_arc_expr;
  std::string data_arc_expr;
  void compute_setting(const Value &src,
                       const Value &dest,
                       const Value &tag) {

    using namespace std::placeholders;
    auto store = std::bind(store_if_not<std::string>, _1, _2, "");

    std::vector<std::string> types;
    store(types, value_to_type(src));
    store(types, value_to_type(dest));
    store(types, value_to_type(tag));
    setting_type = format_tuple(types);

    std::vector<std::string> names;
    store(names, value_to_str(src, "src", false));
    store(names, value_to_str(dest, "dest", false));
    store(names, value_to_str(tag, "tag", false));
    setting_arc_expr = format_tuple(names);
    errs() << "setting arc expr:  " << setting_arc_expr << "\n";

    // TODO: generator for  MessageRequest
  }

private:

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

  const ID id;
  std::string name_prefix;
  Place &send_setting; // INPUT
  Place &send_data;    // INPUT
  Place &send_reqst;   // OUTPUT
  Transition &send;

public:

  CN_MPI_Isend(const CallSite &cs/* TODO: , const Value &rank */)
    : id(generate_id()),
      name_prefix("send" + std::to_string(id)),
      send_setting(*add_place("<empty>", "", name_prefix + "_setting")),
      send_data(*add_place("<empty>", "", name_prefix + "_data")),
      send_reqst(*add_place("(MPI_Request, MessageRequest)", "", name_prefix + "_reqst")),
      send(*add_transition({}, name_prefix)) {

    Value *dest = cs.getArgument(3);
    Value *tag = cs.getArgument(4);
    compute_setting(*dest /* TODO: pass rank */, *dest, *tag);
    send_setting.type = setting_type;

  }

  // provide the ID back to generator and create an annotation for the corresponding instruction
  ID get_id() {
    return id;
  }
};

struct CommNetFactory {

  CommNetFactory() = delete;

  static PluginCommNet createCommNet(const CallSite &cs) {
    Function *f = cs.getCalledFunction();
    assert (f->hasName() && "The CommNetFactory expects call site with a named function");

    StringRef call_name = f->getName();
    if (call_name == "MPI_Isend" || call_name == "MPI_Send" /* TODO: remove MPI_Send */) {
      return CN_MPI_Isend(cs);
    }
    return PluginCommNet();
  }
};

} // end of anonymous namespace
#endif // COMM_NET_FACTORY_H
