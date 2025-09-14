#ifndef DAS_CHECKER_TEST_BASE_H
#define DAS_CHECKER_TEST_BASE_H

#include <Das/checker_interface.h>
#include <Das/scheme.h>

namespace Das {

class Checker_Test_Base : public Scheme, public Checker::Manager_Interface
{
    Q_OBJECT
public:
    Checker_Test_Base();
    virtual ~Checker_Test_Base() = default;

    bool is_server_connected() const override;

protected:
    bool load(const QString& file_path);

    Plugin_Type* _pl_type = nullptr;
};

} // namespace Das

#endif // DAS_CHECKER_TEST_BASE_H
