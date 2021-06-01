/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_BINARYEN_ACCOUNTNONCEAPI_IMPL_HPP
#define KAGOME_RUNTIME_BINARYEN_ACCOUNTNONCEAPI_IMPL_HPP

#include "runtime/account_nonce_api.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"

namespace kagome::runtime::binaryen {

  class AccountNonceApiImpl final : public RuntimeApi, public AccountNonceApi {
   public:
    AccountNonceApiImpl(
        const std::shared_ptr<RuntimeCodeProvider> &wasm_provider,
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory);

    outcome::result<primitives::AccountNonce> account_nonce(
        const primitives::AccountId &account_id) override;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_RUNTIME_BINARYEN_ACCOUNTNONCEAPI_IMPL_HPP
