
#ifndef MORPH_PLAIN_TEXT_FMT
#define MORPH_PLAIN_TEXT_FMT

#include "morpheus/Formats/Formatter.hpp"

#include "llvm/Support/raw_ostream.h"

using namespace std;

namespace cn {
  namespace formats {

    namespace { // local utilities
      template <typename T>
      auto get_print_element_f_(ostream &os, const Formatter &fmt, string delim="", size_t pos=0) {
        return [&, pos] (const std::unique_ptr<T> &e) {
          os << std::string(pos, ' ');
          fmt.format(os, *e);
          os << delim;
        };
      }

      template <typename T, typename UnaryPredicate>
      auto get_print_element_f_(ostream &os, const formats::Formatter &fmt,
                                UnaryPredicate pred, string delim="", size_t pos=0) {
        return [&, pred, pos] (const unique_ptr<T> &e) {
          if (pred(*e)) {
            os << std::string(pos, ' ');
            fmt.format(os, *e);
            os << delim;
          }
        };
      }

      template <EdgeCategory C>
      using EdgePredicate = CommunicationNet::EdgePredicate<C>;
    } // end of local utilities


    // =========================================================================
    // PlainText Formatter

    class PlainText final : public Formatter {

    public:
      ostream& format(ostream &os, const NetElement &net_elem) const {
        if (net_elem.name.empty()) {
          os << this; // print pointer value
        } else {
          os << net_elem.name;
        }
        return os;
      }

      ostream& format(ostream &os, const Edge &edge) const {
        format(os, edge.startpoint);
        if (edge.arc_expr.empty()) {
          os << " -> ";
        } else {
          os << " --/ " << edge.arc_expr << " /--> ";
        }
        // os << edge.endpoint;
        format(os, edge.endpoint);
        return os;
      }

      ostream& format(ostream &os, const Place &place) const {
        os << "P(" << place.get_id() << "): ";

        format(os, static_cast<const NetElement&>(place));

        os << "<";
        if (!place.type.empty()) {
          os << place.type;
        }
        os << ">";

        os << "[";
        if (!place.init_expr.empty()) {
          os << place.init_expr;
        }
        os << "]";
        return os;
      }

      ostream& format(ostream &os, const Transition &transition) const {
        os << "T(" << transition.get_id() << "): ";

        format(os, static_cast<const NetElement&>(transition));

        os << pp_vector(transition.guard, ", ", "[", "]");
        return os;
      }


      ostream& format(ostream &os, const CommunicationNet &cn) const {
        os << "CommunicationNet(" << cn.get_id() << "):\n";

        os << "Places:\n";
        auto places = cn.places();
        std::for_each(places.begin(),
                      places.end(),
                      get_print_element_f_<Place>(os, *this, "\n", 2));

        os << "Transitions:\n";
        auto transitions = cn.transitions();
        std::for_each(transitions.begin(),
                      transitions.end(),
                      get_print_element_f_<Transition>(os, *this, "\n", 2));


        os << "Input edges:\n";
        for (const auto &p : cn.places()) {
          std::for_each(
            p->leads_to.begin(),
            p->leads_to.end(),
            get_print_element_f_<Edge>(os, *this, EdgePredicate<REGULAR>(), "\n", 2));
        }

        os << "Outuput edges:\n";
        for (const auto &t : cn.transitions()) {
          std::for_each(t->leads_to.begin(),
                        t->leads_to.end(),
                        get_print_element_f_<Edge>(os, *this, EdgePredicate<REGULAR>(), "\n", 2));
        }

        os << "CF edges: \n";
        for (const auto &p : cn.places()) {
          std::for_each(p->leads_to.begin(),
                        p->leads_to.end(),
                        get_print_element_f_<Edge>(os, *this, EdgePredicate<CONTROL_FLOW>(), "\n", 2));
        }
        for (const auto &t : cn.transitions()) {
          std::for_each(t->leads_to.begin(),
                        t->leads_to.end(),
                        get_print_element_f_<Edge>(os, *this, EdgePredicate<CONTROL_FLOW>(), "\n", 2));
        }
        return os;
      }
    };

  } // end of formats namespace

  template <typename T>
  raw_ostream &operator<< (llvm::raw_ostream &os, const Printable<T> &printable) {
    std::stringstream ss;
    printable.print(ss, formats::PlainText());
    os << ss.str();
    return os;
  }
} // end of cn namespace

# endif // MORPH_PLAIN_TEXT_FMT
