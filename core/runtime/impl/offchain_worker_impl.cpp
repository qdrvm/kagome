/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/offchain_worker_impl.hpp"

namespace kagome::runtime {
  OffchainWorkerImpl::OffchainWorkerImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::Extension> &extension)
      : RuntimeApi(wasm_provider, extension) {}

  outcome::result<void> OffchainWorkerImpl::offchain_worker(BlockNumber bn) {
    return execute<void>("OffchainWorkerApi_offchain_worker", bn);
  }
}  // namespace kagome::runtime
