/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_OFFCHAIN_WORKER_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_OFFCHAIN_WORKER_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/offchain_worker.hpp"

namespace kagome::runtime::binaryen {

  class OffchainWorkerImpl : public RuntimeApi, public OffchainWorker {
   public:
    OffchainWorkerImpl(
        const std::shared_ptr<RuntimeCodeProvider> &wasm_provider,
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory);

    ~OffchainWorkerImpl() override = default;

    outcome::result<void> offchain_worker(BlockNumber bn) override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_IMPL_HPP
