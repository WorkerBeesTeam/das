#include <QDebug>


#include <jwt-cpp/include/jwt-cpp/jwt.h>

#include "jwt_helper.h"

namespace Das {

JWT_Helper::JWT_Helper(const std::string& secret_key)
{
    if (secret_key.empty())
        qCritical() << "JWT ERROR: need secret key";
    algo_ = new jwt::algorithm::hs256{secret_key};
//    token_verifier_ = new jwt::verifier<jwt::default_clock>{{}};
//    token_verifier_->allow_algorithm(jwt::algorithm::hs256{secret_key});

    auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret_key});
    token_verifier_ = new jwt::verifier<jwt::default_clock>{std::move(verifier)};
}

JWT_Helper::~JWT_Helper()
{
    delete token_verifier_;
    delete algo_;
}

std::string JWT_Helper::create(uint32_t user_id, const std::set<uint32_t>& user_scheme_group_set) const
{
    picojson::value::array user_scheme_group_array;
    for (uint32_t scheme_group_id: user_scheme_group_set)
        user_scheme_group_array.push_back(picojson::value{static_cast<double>(scheme_group_id)});

    auto token = jwt::create()
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
        .set_payload_claim("user_id", picojson::value{static_cast<double>(user_id)})
        .set_payload_claim("scheme_groups", picojson::value{user_scheme_group_array})
        .sign(*algo_);
    return token;
}

std::string JWT_Helper::decode_and_verify(const std::string& token) const
{
    auto decoded = jwt::decode(token);
//    std::cout << "T type " << decoded.get_type() << std::endl;
//    for (auto it: decoded.get_payload_claims())
//        std::cout << "T pl claim " << it.first << std::endl;
    verify(decoded);
    return decoded.get_payload();
}

void JWT_Helper::verify(const jwt::decoded_jwt& jwt) const
{
    token_verifier_->verify(jwt);
}

} // namespace Das
