#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "rest.h"
#include "multipart_form_data_parser.h"

namespace Das {
namespace Rest {

Multipart_Form_Data_Parser::Multipart_Form_Data_Parser(const served::request &req) :
    req_(req)
{
    //         "multipart/form-data; "
    //         "boundary=------------------------000000000000000; "
    //         "boundary=------------------------14609ac75d1a8714";
    const std::string content_type = req.header("Content-Type");

    std::vector<std::string> content_type_list;
    boost::split(content_type_list, content_type, boost::is_any_of("; "));
    boost::to_lower(content_type_list.front());
    if (content_type_list.front() == "multipart/form-data")
    {
        //            "--------------------------14609ac75d1a8714\r\n"
        //            "Content-Disposition: attachment; name=\"myfilename\"; filename=\"test.bin\"\r\n"
        //            "Content-Type: application/octet-stream\r\n\r\n"
        //            "DATA\r\n"
        //            "--------------------------14609ac75d1a8714--\r\n";
        const std::string body = req.body();

        std::vector<std::string> pv_list;
        for (auto it = ++content_type_list.cbegin(); it != content_type_list.cend(); ++it)
        {
            pv_list.clear();
            boost::split(pv_list, *it, [](char c){ return c == '='; });
            boost::to_lower(pv_list.front());
            if (pv_list.front() == "boundary")
            {
                parse_part(body, pv_list.back());
            }
        }
    }

    qCDebug(Rest_Log) << "write_item_file" << req.body().size() << QString::fromStdString(content_type);
}

void Multipart_Form_Data_Parser::parse_part(const std::string &body, const std::string &boundary)
{
    std::size_t pos = body.find(boundary);
    if (pos != std::string::npos)
    {
        pos += boundary.size() + 2; // \r\n
        std::size_t end_pos = body.find(boundary, pos);
        if (end_pos != std::string::npos)
        {
            end_pos -= 4; // \r\n--
            const std::string rn = "\r\n";

            pos = body.find(rn, pos);
            if (pos != std::string::npos)
            {
                /* TODO:
                     */
                body.substr(0, pos);
            }
        }
    }
}


} // namespace Rest
} // namespace Das
