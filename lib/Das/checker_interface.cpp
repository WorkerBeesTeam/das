#include "checker_interface.h"

namespace Das {
namespace Checker {

void Manager_Interface::init_checker(Interface *checker, Scheme *scheme)
{
    checker->mng_ = this;
    checker->scheme_ = scheme;
}

// ------------------------------------------------------------------------------

Interface::Interface() :
    mng_(nullptr),
    scheme_(nullptr)
{
}

Manager_Interface *Interface::manager() { return mng_; }
Scheme *Interface::scheme() { return scheme_; }

} // namespace Checker
} // namespace Das
