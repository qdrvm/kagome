/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_OFFCHAIN_WORKER_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_OFFCHAIN_WORKER_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/offchain_worker.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime::binaryen {

  class OffchainWorkerImpl : public RuntimeApi, public OffchainWorker {
   public:
    OffchainWorkerImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::ExtensionFactory> &extension_factory);

    ~OffchainWorkerImpl() override = default;

    outcome::result<void> offchain_worker(BlockNumber bn) override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_IMPL_HPP
