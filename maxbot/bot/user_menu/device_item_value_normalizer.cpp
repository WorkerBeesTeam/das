#include <cmath>

#include "device_item_value_normalizer.h"

namespace Das {
namespace Bot {
namespace User_Menu {

template <typename T>
struct to_str : public exprtk::igeneric_function<T>
{
    typedef exprtk::igeneric_function<T> igenfunct_t;
    typedef typename igenfunct_t::parameter_list_t parameter_list_t;
    typedef typename igenfunct_t::generic_type generic_t;
    typedef typename generic_t::scalar_view scalar_t;

    to_str() :
        exprtk::igeneric_function<T>("T", igenfunct_t::e_rtrn_string)
    {}

    inline T operator()(std::string& result, parameter_list_t parameters)
    {
        scalar_t scalar(parameters[0]);
        result = impl(scalar());
        return T(0);
    }

    static std::string impl(T value)
    {
        std::string result = std::to_string(value);

        auto it = std::find_if(result.cbegin(), result.cend(), [](const char i) {
            return i == '.' || i == ',';
        });

        if (it != result.cend())
        {
            result.erase(result.find_last_not_of('0') + 1, std::string::npos);
            if (!result.empty() && (result.back() == '.' || result.back() == ','))
                result.resize(result.size() - 1);
        }
        return result;
    }
};

// ----------------------------------------------------

Device_Item_Value_Normalizer::Device_Item_Value_Normalizer(const std::string &expression_string)
{
    parse_expression(expression_string);
}

std::string Device_Item_Value_Normalizer::normalize(Expr_T value)
{
    _expression_result.clear();
    _expression_var = value;

    Expr_T result = _expression.value();

    if (_expression_result.empty())
    {
        if (!std::isnan(result))
            _expression_var = result;
        _expression_result = to_str<Expr_T>::impl(_expression_var);
    }

    return _expression_result;
}

void Device_Item_Value_Normalizer::parse_expression(const std::string &expression_string)
{
    static to_str<Expr_T> to_str_func;

    exprtk::symbol_table<Expr_T> symbol_table;
    symbol_table.add_function("to_str", to_str_func);
    symbol_table.add_variable("x", _expression_var);
    symbol_table.add_stringvar("res", _expression_result);
    symbol_table.add_constants();

    _expression.register_symbol_table(symbol_table);

    exprtk::parser<Expr_T> parser;
    if (!parser.compile(expression_string, _expression))
        throw std::runtime_error("Compilation error! " + parser.error());
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
