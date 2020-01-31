#ifndef DAS_DATABASE_SCHEMED_MODEL_H
#define DAS_DATABASE_SCHEMED_MODEL_H

#include <stdint.h>

namespace Das {
namespace Database {

class Schemed_Model
{
public:
    Schemed_Model(uint32_t scheme_id = default_scheme_id());

    static uint32_t default_scheme_id();
    static void set_default_scheme_id(uint32_t scheme_id);

    uint32_t scheme_id() const;
    void set_scheme_id(uint32_t scheme_id);

private:
    static uint32_t default_scheme_id_;

    uint32_t scheme_id_;
};

} // namespace Database
} // namespace Das

#endif // DAS_DATABASE_SCHEMED_MODEL_H
