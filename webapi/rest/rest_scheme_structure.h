#ifndef DAS_REST_SCHEME_STRUCTURE_H
#define DAS_REST_SCHEME_STRUCTURE_H

#include <served/served.hpp>

namespace Das {
namespace Rest {

class Scheme_Structure
{
public:
    Scheme_Structure(served::multiplexer& mux, const std::string& scheme_path);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_SCHEME_STRUCTURE_H
