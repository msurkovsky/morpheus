
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
  class CommunicationNet;

  namespace formats {

    struct Formatter {
      virtual std::ostream& format(std::ostream &os, const NetElement &) const = 0;
      virtual std::ostream& format(std::ostream &os, const Edge &) const = 0;
      virtual std::ostream& format(std::ostream &os, const Place &) const = 0;
      virtual std::ostream& format(std::ostream &os, const Transition &) const = 0;
      virtual std::ostream& format(std::ostream &os, const CommunicationNet &) const = 0;
    };

  } // end of formats
} // end of cn

#endif // MORPH_FMT
