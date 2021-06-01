/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/account_nonce_api.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmAccountNonceApi::WavmAccountNonceApi(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::AccountNonce> WavmAccountNonceApi::account_nonce(
      const primitives::AccountId &account_id) {
    return executor_->callAtLatest<primitives::AccountNonce>(
        "AccountNonceApi_account_nonce", account_id);
  };

}  // namespace kagome::runtime::wavm
