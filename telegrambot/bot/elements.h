#ifndef DAS_BOT_ELEMENTS_H
#define DAS_BOT_ELEMENTS_H

#include <tgbot/types/Message.h>

#include "menu_item.h"

namespace Das {
namespace Bot {

class Elements : public Menu_Item
{
public:
    Elements(const Bot_Base &controller, uint32_t user_id, const Scheme_Item &scheme,
                 const std::vector<std::string> &cmd, const std::string& msg_data);

    void generate_answer();
    void process_user_data(const std::string& text);
private:
    void fill_sections();
    void fill_section(uint32_t sct_id);
    void fill_groups(uint32_t sct_id);
    void fill_group(uint32_t dig_id);
    void fill_group_info(uint32_t dig_id);
    void fill_group_mode(uint32_t dig_id);
    void fill_group_param(uint32_t dig_id);

    const std::string msg_data_;
    std::string back_data_;
    QString scheme_suffix_;

    std::vector<std::string>::const_iterator begin_;
    const std::vector<std::string> cmd_;
};

} // namespace Bot
} // namespace Das

#endif // DAS_BOT_ELEMENTS_H
