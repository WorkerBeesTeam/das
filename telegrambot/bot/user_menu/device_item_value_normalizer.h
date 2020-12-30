#ifndef DAS_BOT_USER_MENU_DEVICE_ITEM_VALUE_NORMALIZER_H
#define DAS_BOT_USER_MENU_DEVICE_ITEM_VALUE_NORMALIZER_H

//#include
#include "exprtk.hpp"

namespace Das {
namespace Bot {
namespace User_Menu {

class Device_Item_Value_Normalizer
{
public:
    Device_Item_Value_Normalizer(const std::string& expression_string);

    using Expr_T = double;
    std::string normalize(Expr_T value);
private:
    void parse_expression(const std::string& expression_string);
    double _expression_var;
    std::string _expression_result;
    exprtk::expression<Expr_T> _expression;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_DEVICE_ITEM_VALUE_NORMALIZER_H
