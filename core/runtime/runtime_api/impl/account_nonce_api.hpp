/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
