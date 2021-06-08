/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/offchain_worker.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmOffchainWorker::WavmOffchainWorker(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<void> WavmOffchainWorker::offchain_worker(
      BlockNumber bn) {
    // TODO(Harrm): Perhaps should be invoked on a state of block bn
    return executor_->callAtLatest<void>("OffchainWorker_offchain_worker", bn);
  }
}  // namespace kagome::runtime::wavm
