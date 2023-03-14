/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SERVICE_STATE_REQUESTS_GET_READ_PROOF_HPP
#define KAGOME_API_SERVICE_STATE_REQUESTS_GET_READ_PROOF_HPP

#include "api/service/base_request.hpp"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include "api/jrpc/value_converter.hpp"
#include "api/service/state/state_api.hpp"

namespace kagome::api {
  inline jsonrpc::Value makeValue(const StateApi::ReadProof &proof) {
    std::vector<jsonrpc::Value> j_proof;
    boost::range::push_back(
        j_proof,
        proof.proof
            | boost::adaptors::transformed(
                [](const common::Buffer &proof) { return makeValue(proof); }));
    return jsonrpc::Value::Struct{
        std::pair{"at", makeValue(common::hex_lower_0x(proof.at))},
        std::pair{"proof", makeValue(j_proof)},
    };
  }
}  // namespace kagome::api

namespace kagome::api::state::request {
  class GetReadProof final
      : public details::RequestType<StateApi::ReadProof,
                                    std::vector<std::string>,
                                    std::optional<std::string>> {
   public:
    explicit GetReadProof(std::shared_ptr<StateApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    }

    outcome::result<StateApi::ReadProof> execute() override {
      std::vector<common::Buffer> keys;
      keys.reserve(getParam<0>().size());
      for (auto &str_key : getParam<0>()) {
        OUTCOME_TRY(key, kagome::common::unhexWith0x(str_key));
        keys.emplace_back(std::move(key));
      }
      std::optional<primitives::BlockHash> at{};
      if (auto opt_at = getParam<1>(); opt_at.has_value()) {
        OUTCOME_TRY(at_,
                    primitives::BlockHash::fromHexWithPrefix(opt_at.value()));
        at = std::move(at_);
      }
      return api_->getReadProof(keys, at);
    }

   private:
    std::shared_ptr<StateApi> api_;
  };
}  // namespace kagome::api::state::request

#endif  // KAGOME_API_SERVICE_STATE_REQUESTS_GET_READ_PROOF_HPP
