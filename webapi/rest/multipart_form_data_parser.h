#ifndef DAS_REST_MULTIPART_FORM_DATA_PARSER_H
#define DAS_REST_MULTIPART_FORM_DATA_PARSER_H

#include <served/served.hpp>

namespace Das {
namespace Rest {

class Multipart_Form_Data_Parser
{
public:
    Multipart_Form_Data_Parser(const served::request& req);
private:
    void parse_part(const std::string& body, const std::string& boundary);
    const served::request& req_;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_MULTIPART_FORM_DATA_PARSER_H
