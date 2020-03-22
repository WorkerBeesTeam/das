#ifndef DAS_BOT_SCHEME_ITEM_H
#define DAS_BOT_SCHEME_ITEM_H


#include <plus/das/scheme_info.h>

namespace Das {
namespace Bot {

struct Scheme_Item : public Scheme_Info
{
    using Scheme_Info::Scheme_Info;
    QString title_;
};

} // namespace Bot
} // namespace Das

#endif // DAS_BOT_SCHEME_ITEM_H
