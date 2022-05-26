/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/offchain_worker_pool_impl.hpp"

namespace kagome::offchain {

  using soralog::util::getThreadNumber;

  OffchainWorkerPoolImpl::OffchainWorkerPoolImpl()
      : log_(log::createLogger("OffchainWorkerPool", "offchain")) {
    BOOST_ASSERT(log_);
  }

  void OffchainWorkerPoolImpl::addWorker(std::shared_ptr<OffchainWorker> ofw) {
    std::lock_guard<std::mutex> lk(mut_);
    offchain_workers_.emplace(getThreadNumber(), std::move(ofw));
  }

  std::optional<std::shared_ptr<OffchainWorker>>
  OffchainWorkerPoolImpl::getWorker() {
    std::lock_guard<std::mutex> lk(mut_);
    if (offchain_workers_.count(getThreadNumber()) != 0) {
      return offchain_workers_.at(getThreadNumber());
    }
    return std::nullopt;
  }

  bool OffchainWorkerPoolImpl::removeWorker() {
    std::lock_guard<std::mutex> lk(mut_);
    return offchain_workers_.erase(getThreadNumber()) == 1;
  }

}  // namespace kagome::offchain
