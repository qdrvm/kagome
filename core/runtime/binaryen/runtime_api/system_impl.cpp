/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/system_impl.hpp"

namespace kagome::runtime::binaryen {

  SystemImpl::SystemImpl(const std::shared_ptr<WasmProvider> &wasm_provider,
                         const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(wasm_provider, runtime_manager) {}

  outcome::result<primitives::AccountNonce> SystemImpl::account_nonce(
      const primitives::AccountId &account_id) {
    return execute<primitives::AccountNonce>(
        "AccountNonceApi_account_nonce", CallPersistency::EPHEMERAL, account_id);
  }

}  // namespace kagome::runtime::binaryen
