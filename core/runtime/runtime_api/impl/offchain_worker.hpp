/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_HPP
#define KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_HPP

#include "runtime/runtime_api/offchain_worker.hpp"

namespace kagome::runtime {

  class Executor;

  class OffchainWorkerImpl final : public OffchainWorker {
   public:
    explicit OffchainWorkerImpl(std::shared_ptr<Executor> executor);

    outcome::result<void> offchain_worker(BlockNumber bn) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_OFFCHAIN_WORKER_HPP
