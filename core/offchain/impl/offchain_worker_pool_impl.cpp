/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
    offchain_workers_.exclusiveAccess([&](auto &offchain_workers) {
      offchain_workers.emplace(getThreadNumber(), std::move(ofw));
    });
  }

  std::optional<std::shared_ptr<OffchainWorker>>
  OffchainWorkerPoolImpl::getWorker() const {
    return offchain_workers_.sharedAccess(
        [](const auto &offchain_workers)
            -> std::optional<std::shared_ptr<OffchainWorker>> {
          if (auto it = offchain_workers.find(getThreadNumber());
              it != offchain_workers.end()) {
            return it->second;
          }
          return std::nullopt;
        });
  }

  bool OffchainWorkerPoolImpl::removeWorker() {
    return offchain_workers_.exclusiveAccess([&](auto &offchain_workers) {
      return offchain_workers.erase(getThreadNumber()) == 1;
    });
  }

}  // namespace kagome::offchain
