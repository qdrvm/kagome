/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  OffchainWorkerImpl::OffchainWorkerImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<void> OffchainWorkerImpl::offchain_worker(
      primitives::BlockInfo block_info) {
    return executor_->callAt<void>(
        block_info.hash, "OffchainWorker_offchain_worker", block_info.number);
  }
}  // namespace kagome::runtime
