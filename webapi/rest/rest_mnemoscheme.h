#ifndef DAS_REST_MNEMOSCHEME_H
#define DAS_REST_MNEMOSCHEME_H

#include <served/multiplexer.hpp>

namespace Das {
namespace Rest {

class Mnemoscheme
{
public:
    Mnemoscheme(served::multiplexer& mux, const std::string& scheme_path);
private:
    void get_list(served::response& res, const served::request& req);
    void get_item(served::response& res, const served::request& req);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_MNEMOSCHEME_H
