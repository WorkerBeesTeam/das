#ifndef DAS_MODBUS_PACK_H
#define DAS_MODBUS_PACK_H

#include <QModbusDataUnit>

#include <Das/write_cache_item.h>

#include "config.h"

QT_FORWARD_DECLARE_CLASS(QModbusClient)
QT_FORWARD_DECLARE_CLASS(QModbusReply)

namespace Das {
namespace Modbus {

template <typename T>
struct Pack_Item_Cast {
    static inline Device_Item* run(T item) { return item; }
};

template <>
struct Pack_Item_Cast<Write_Cache_Item> {
    static inline Device_Item* run(const Write_Cache_Item& item) { return item.dev_item_; }
};

template<typename T>
struct Pack
{
    Pack(Pack<T>&& o) = default;
    Pack(const Pack<T>& o) = default;
    Pack<T>& operator =(Pack<T>&& o) = default;
    Pack<T>& operator =(const Pack<T>& o) = default;
    Pack(T&& item)
        : _reply(nullptr)
    {
        init(Pack_Item_Cast<T>::run(item), std::is_same<Write_Cache_Item, T>::value);
        _items.push_back(std::move(item));
    }

    void init(Device_Item* dev_item, bool is_write)
    {
        bool ok;
        _server_address = Config::address(dev_item->device(), &ok);
        if (ok && _server_address > 0)
        {
            _start_address = Config::unit(dev_item, &ok);
            if (ok && _start_address >= 0)
            {
                if (dev_item->register_type() > QModbusDataUnit::Invalid && dev_item->register_type() <= QModbusDataUnit::HoldingRegisters &&
                        (!is_write || (dev_item->register_type() == QModbusDataUnit::Coils ||
                                       dev_item->register_type() == QModbusDataUnit::HoldingRegisters)))
                {
                    _register_type = static_cast<QModbusDataUnit::RegisterType>(dev_item->register_type());
                }
                else
                    _register_type = QModbusDataUnit::Invalid;
            }
            else
                _start_address = -1;
        }
        else
            _server_address = -1;
    }

    bool is_valid() const
    {
        return _server_address > 0 && _start_address >= 0 && _register_type != QModbusDataUnit::Invalid;
    }

    bool add_next(T&& item)
    {
        Device_Item* dev_item = Pack_Item_Cast<T>::run(item);
        if (_register_type == dev_item->register_type() &&
            _server_address == Config::address(dev_item->device()))
        {
            int unit = Config::unit(dev_item);
            if (unit == (_start_address + static_cast<int>(_items.size())))
            {
                _items.push_back(std::move(item));
                return true;
            }
        }
        return false;
    }

    bool assign(Pack<T>& pack)
    {
        if (_register_type == pack._register_type
            && _server_address == pack._server_address
            && (_start_address + static_cast<int>(_items.size())) == pack._start_address)
        {
            std::copy( std::make_move_iterator(pack._items.begin()),
                       std::make_move_iterator(pack._items.end()),
                       std::back_inserter(_items) );
            return true;
        }
        return false;
    }

    bool operator <(Device_Item* dev_item) const
    {
        return _register_type < dev_item->register_type() ||
               _server_address < Config::address(dev_item->device()) ||
               (_start_address + static_cast<int>(_items.size())) < Config::unit(dev_item);
    }

    int _server_address;
    int _start_address;
    QModbusDataUnit::RegisterType _register_type;
    QModbusReply* _reply;

    std::vector<T> _items;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PACK_H
