/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_OFFCHAIN_WORKER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_OFFCHAIN_WORKER_HPP

#include "runtime/offchain_worker.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmOffchainWorker final: public OffchainWorker {
   public:
    WavmOffchainWorker(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<void> offchain_worker(BlockNumber bn) override {
      // TODO(Harrm): Perhaps should be invoked on a state of block bn
      return executor_->callAtLatest<void>(
          "OffchainWorker_offchain_worker", bn);
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_OFFCHAIN_WORKER_HPP
