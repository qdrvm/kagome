/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "api/service/base_request.hpp"
#include "api/service/payment/payment_api.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/runtime_dispatch_info.hpp"

namespace kagome::api::payment::request {

  class QueryInfo final
      : public details::RequestType<
            primitives::RuntimeDispatchInfo<primitives::Weight>,
            std::string,
            std::optional<std::string>> {
   public:
    explicit QueryInfo(std::shared_ptr<PaymentApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    }

    outcome::result<primitives::RuntimeDispatchInfo<primitives::Weight>>
    execute() override {
      auto ext_hex = getParam<0>();
      OUTCOME_TRY(ext_bytes, common::unhexWith0x(ext_hex));

      OUTCOME_TRY(extrinsic, scale::decode<primitives::Extrinsic>(ext_bytes));

      auto at_hex_opt = getParam<1>();
      if (not at_hex_opt.has_value()) {
        return api_->queryInfo(extrinsic, ext_bytes.size(), std::nullopt);
      }
      common::Hash256 at_hash;
      OUTCOME_TRY(at, common::unhexWith0x(at_hex_opt.value()));
      BOOST_ASSERT(at.size() == common::Hash256::size());
      std::copy_n(at.cbegin(), common::Hash256::size(), at_hash.begin());
      return api_->queryInfo(extrinsic, ext_bytes.size(), std::cref(at_hash));
    }

   private:
    std::shared_ptr<PaymentApi> api_;
  };

}  // namespace kagome::api::payment::request
