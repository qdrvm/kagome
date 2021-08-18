/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_ACCOUNTNONCEAPI_HPP
#define KAGOME_RUNTIME_IMPL_ACCOUNTNONCEAPI_HPP

#include "runtime/runtime_api/account_nonce_api.hpp"

namespace kagome::runtime {

  class Executor;

  class AccountNonceApiImpl final : public AccountNonceApi {
   public:
    explicit AccountNonceApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<primitives::AccountNonce> account_nonce(
        const primitives::BlockHash &block,
        const primitives::AccountId &account_id) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_ACCOUNTNONCEAPI_HPP
