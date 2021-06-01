/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/offchain_worker_impl.hpp"

namespace kagome::runtime::binaryen {
  OffchainWorkerImpl::OffchainWorkerImpl(
      const std::shared_ptr<RuntimeCodeProvider> &wasm_provider,
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory)
      : RuntimeApi(runtime_env_factory) {}

  outcome::result<void> OffchainWorkerImpl::offchain_worker(BlockNumber bn) {
    return execute<void>("OffchainWorkerApi_offchain_worker",
                         CallConfig{.persistency = CallPersistency::EPHEMERAL},
                         bn);
  }
}  // namespace kagome::runtime::binaryen
