/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/account_nonce_api_impl.hpp"

namespace kagome::runtime::binaryen {

  AccountNonceApiImpl::AccountNonceApiImpl(
      const std::shared_ptr<RuntimeCodeProvider> &wasm_provider,
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory)
      : RuntimeApi(runtime_env_factory) {}

  outcome::result<primitives::AccountNonce> AccountNonceApiImpl::account_nonce(
      const primitives::AccountId &account_id) {
    return execute<primitives::AccountNonce>(
        "AccountNonceApi_account_nonce",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        account_id);
  }

}  // namespace kagome::runtime::binaryen
