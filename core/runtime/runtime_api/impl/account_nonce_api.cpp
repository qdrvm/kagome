/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/account_nonce_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  AccountNonceApiImpl::AccountNonceApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::AccountNonce> AccountNonceApiImpl::account_nonce(
      const primitives::BlockHash &block,
      const primitives::AccountId &account_id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<primitives::AccountNonce>(
        ctx, "AccountNonceApi_account_nonce", account_id);
  };

}  // namespace kagome::runtime
