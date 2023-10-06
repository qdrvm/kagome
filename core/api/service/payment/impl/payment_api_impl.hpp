/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PAYMENT_API_IMPL_HPP
#define KAGOME_PAYMENT_API_IMPL_HPP

#include "api/service/payment/payment_api.hpp"

namespace kagome::runtime {
  class TransactionPaymentApi;
}

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::api {

  class PaymentApiImpl : public PaymentApi {
   public:
    PaymentApiImpl(std::shared_ptr<runtime::TransactionPaymentApi> api,
                   std::shared_ptr<const blockchain::BlockTree> block_tree);
    ~PaymentApiImpl() override = default;

    outcome::result<primitives::RuntimeDispatchInfo<primitives::OldWeight>>
    queryInfo(const primitives::Extrinsic &extrinsic,
              uint32_t len,
              OptionalHashRef at) const override;

   private:
    std::shared_ptr<runtime::TransactionPaymentApi> api_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
  };

}  // namespace kagome::api

#endif  // KAGOME_PAYMENT_API_IMPL_HPP
