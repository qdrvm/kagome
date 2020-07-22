/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/offchain_worker_impl.hpp"

namespace kagome::runtime::binaryen {
  OffchainWorkerImpl::OffchainWorkerImpl(
      std::shared_ptr<WasmProvider> wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(std::move(wasm_provider), runtime_manager) {}

  outcome::result<void> OffchainWorkerImpl::offchain_worker(BlockNumber bn) {
    return execute<void>(
        "OffchainWorkerApi_offchain_worker", CallPersistency::EPHEMERAL, bn);
  }
}  // namespace kagome::runtime::binaryen
