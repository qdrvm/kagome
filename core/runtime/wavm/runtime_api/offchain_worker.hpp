/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_OFFCHAIN_WORKER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_OFFCHAIN_WORKER_HPP

#include "runtime/offchain_worker.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmOffchainWorker final : public OffchainWorker {
   public:
    WavmOffchainWorker(std::shared_ptr<Executor> executor);

    outcome::result<void> offchain_worker(BlockNumber bn) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_OFFCHAIN_WORKER_HPP
