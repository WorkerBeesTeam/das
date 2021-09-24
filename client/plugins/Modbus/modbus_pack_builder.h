#ifndef DAS_MODBUS_PACK_BUILDER_H
#define DAS_MODBUS_PACK_BUILDER_H

#include "modbus_pack.h"

namespace Das {
namespace Modbus {

template<typename T, typename Container> struct Input_Container_Device_Item_Type { typedef T& type; };
template<typename T, typename Container> struct Input_Container_Device_Item_Type<T, const Container> { typedef T type; };

template<typename T>
class Pack_Builder
{
public:
    template<typename Input_Container>
    Pack_Builder(Input_Container& items)
    {
        typename std::vector<Pack<T>>::iterator it;

        for (typename Input_Container_Device_Item_Type<T, Input_Container>::type item: items)
            insert(std::move(item));
    }

    std::vector<Pack<T>> _container;
private:
    void insert(T&& item)
    {
        typename std::vector<Pack<T>>::iterator it = _container.begin();
        for (; it != _container.end(); ++it)
        {
            if (*it < Pack_Item_Cast<T>::run(item))
                continue;
            else
            {
                if (it->add_next(std::move(item)))
                    assign_next(it);
                else
                    create(it, std::move(item));
                return;
            }
        }

        if (it == _container.end())
            create(it, std::move(item));
    }

    void create(typename std::vector<Pack<T>>::iterator it, T&& item)
    {
        Pack pack(std::move(item));
        if (pack.is_valid())
        {
            it = _container.insert(it, std::move(pack));
            assign_next(it);
        }
    }

    void assign_next(typename std::vector<Pack<T>>::iterator it)
    {
        if (it != _container.end())
        {
            typename std::vector<Pack<T>>::iterator old_it = it;
            it++;
            if (it != _container.end()
                && old_it->assign(*it))
                _container.erase(it);
        }
    }
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PACK_BUILDER_H
