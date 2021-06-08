/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_ACCOUNTNONCEAPI_HPP
#define KAGOME_RUNTIME_WAVM_ACCOUNTNONCEAPI_HPP

#include "runtime/account_nonce_api.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmAccountNonceApi final: public AccountNonceApi {
   public:
    WavmAccountNonceApi(std::shared_ptr<Executor> executor);

    outcome::result<primitives::AccountNonce> account_nonce(
        const primitives::AccountId &account_id) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_ACCOUNTNONCEAPI_HPP
