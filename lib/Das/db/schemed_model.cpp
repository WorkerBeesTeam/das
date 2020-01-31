#include "schemed_model.h"

namespace Das {
namespace Database {

Schemed_Model::Schemed_Model(uint32_t scheme_id) :
    scheme_id_(scheme_id)
{}

uint32_t Schemed_Model::default_scheme_id_ = 0;
uint32_t Schemed_Model::default_scheme_id() { return default_scheme_id_; }
void Schemed_Model::set_default_scheme_id(uint32_t scheme_id) { default_scheme_id_ = scheme_id; }

uint32_t Schemed_Model::scheme_id() const { return scheme_id_; }
void Schemed_Model::set_scheme_id(uint32_t scheme_id) { scheme_id_ = scheme_id; }

} // namespace Database
} // namespace Das
