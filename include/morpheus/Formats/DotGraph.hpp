
#ifndef MORPH_PLAIN_TEXT_FMT
#define MORPH_PLAIN_TEXT_FMT

#include "morpheus/Formats/Formatter.hpp"

#include <fstream>

using namespace std;

namespace cn {
  namespace formats {

    // =========================================================================
    // DotGraph Formatter

    class DotGraph final : public Formatter {

    public:
      ostream& format(ostream &os, const NetElement &net_elem) const {
        if (!net_elem.name.empty()) {
          os << net_elem.name;
        }
        return os;
      }

      ostream& format(ostream &os, const Edge &edge) const {
        os << edge.startpoint.get_id() << ":box:c"
           << " -> "
           << edge.endpoint.get_id() << ":box:c"
           << " [label=\"" << edge.arc_expr << "\"];";
        return os;
      }

      ostream& format(ostream &os, const Place &place) const {
        string id = place.get_id();

        os << place.get_id()
           << " [shape=plain label=<"
           << "<table border=\"0\">"
            << "<tr>"
             << "<td></td>"
             << "<td align=\"left\" valign=\"top\" rowspan=\"2\">" << place.init_expr << "</td>"
            << "</tr>"

            << "<tr>"
             << "<td port=\"box\" border=\"1\" cellpadding=\"10\" rowspan=\"2\" style=\"rounded\">"; format(os, static_cast<const NetElement&>(place)); os << "</td>"
            << "</tr>"

            << "<tr>"
             << "<td align=\"left\" valign=\"bottom\" rowspan=\"2\">" << place.type << "</td>"
            << "</tr>"

            << "<tr>"
             << "<td></td>"
            << "</tr>"
           << "</table>"
           << ">];";

        return os;
      }

      ostream& format(ostream &os, const Transition &transition) const {
        os << transition.get_id()
           << " [shape=plain label=<"
           << "<table border=\"0\">"
            << "<tr>"
             << "<td border=\"1\" cellpadding=\"10\" port=\"box\">"; format(os, static_cast<const NetElement &>(transition)); os << "</td>"
            << "</tr>"
            << "<tr>"
             << "<td>" << pp_vector(transition.guard, ", ", "[", "]") << "</td>"
            << "</tr>"
           << "</table>"
           << ">];";
        return os;
      }


      ostream& format(ostream &os, const CommunicationNet &cn) const {
        os << "subgraph cluster_CN" << cn.get_id() << "{\n";

        auto places = cn.places();
        std::for_each(places.begin(),
                      places.end(),
                      create_print_fn_<Place>(os, *this, "\n", 0));

        auto transitions = cn.transitions();
        std::for_each(transitions.begin(),
                      transitions.end(),
                      create_print_fn_<Transition>(os, *this, "\n", 0));
        os << "}\n";
        return os;
      }

      ostream& format(ostream &os, const AddressableCN &acn) const {
        os << "digraph ACN" << acn.get_id() << "{\n";
        auto places = acn.places();
        std::for_each(places.begin(),
                      places.end(),
                      create_print_fn_<Place>(os, *this, "\n", 0));

        // format the embedded net
        format(os, acn.embedded_cn);

        // print edges from the embedded CN
        for (auto const &p : acn.embedded_cn.places()) {
          std::for_each(p->leads_to.begin(),
                        p->leads_to.end(),
                        create_print_fn_<Edge>(os, *this, "\n", 0));
        }

        for (auto const &t : acn.embedded_cn.transitions()) {
          std::for_each(t->leads_to.begin(),
                        t->leads_to.end(),
                        create_print_fn_<Edge>(os, *this, "\n", 0));
        }

        // print the edges of the ACN
        for (auto const &p : acn.places()) {
          std::for_each(p->leads_to.begin(),
                        p->leads_to.end(),
                        create_print_fn_<Edge>(os, *this, "\n", 0));
        }

        os << "}";
        return os;
      }
    };

  } // end of formats namespace

  template <typename T>
  ofstream &operator<< (ofstream &fs, const Printable<T> &printable) {
    std::stringstream ss;
    printable.print(ss, formats::DotGraph());
    fs << ss.str();
    return fs;
  }
} // end of cn namespace

# endif // MORPH_PLAIN_TEXT_FMT
