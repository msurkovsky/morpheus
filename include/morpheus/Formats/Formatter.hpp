
#ifndef MORPH_FMT
#define MORPH_FMT

#include "morpheus/ADT/CommunicationNet.hpp"

#include <ostream>
#include <memory>

namespace cn {

  // forward declare the types
  struct NetElement;
  struct Edge;
  struct Place;
  struct Transition;
  class  CommunicationNet;
  struct AddressableCN;

  namespace formats {

    enum struct CN_Type {
      TOP_LEVEL,
      SUBGRAPH,
    };

    struct Formatter {
      virtual std::ostream& format(std::ostream &os, const NetElement &) const = 0;
      virtual std::ostream& format(std::ostream &os, const Edge &) const = 0;
      virtual std::ostream& format(std::ostream &os, const Place &) const = 0;
      virtual std::ostream& format(std::ostream &os, const Transition &) const = 0;
      virtual std::ostream& format(std::ostream &os, const CommunicationNet &) const = 0;
      virtual std::ostream& format(std::ostream &os, const AddressableCN &) const = 0;
    };


    // -------------------------------------------------------------------------
    // utilities

    template <typename T>
    auto create_print_fn_(std::ostream &os, const Formatter &fmt,
                          std::string delim, size_t pos) {
      return [&, delim, pos] (const std::unique_ptr<T> &e) {
        os << std::string(pos, ' ');
        fmt.format(os, *e);
        os << delim;
      };
    }

    template <typename T, typename UnaryPredicate>
    auto create_print_fn_(std::ostream &os, const Formatter &fmt,
                          UnaryPredicate pred, std::string delim, size_t pos) {
      return [&, pred, delim, pos] (const std::unique_ptr<T> &e) {
        if (pred(*e)) {
          os << std::string(pos, ' ');
          fmt.format(os, *e);
          os << delim;
        }
      };
    }

  } // end of formats
} // end of cn

#endif // MORPH_FMT
