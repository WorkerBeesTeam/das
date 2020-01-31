#ifndef DAS_PROTO_SCHEME_H
#define DAS_PROTO_SCHEME_H

#include <functional>

#include "scheme.h"
#include "log/log_pack.h"

namespace Das {

class DAS_LIBRARY_SHARED_EXPORT Proto_Scheme : public Scheme
{
    Q_OBJECT
public:
    using Scheme::Scheme;

    uint32_t id() const;
    void set_id(uint32_t id);

    void passEquipment(std::function<void(Device_item_Group *, Device_Item *)> cb_func);
public slots:
    void setChange(const Log_Value_Item& item);
    void setChangeImpl(uint32_t item_id, const QVariant& raw_value, const QVariant& display_value);
private:
    uint32_t id_ = 0;
};

} // namespace Das

#endif // DAS_PROTO_SCHEME_H
