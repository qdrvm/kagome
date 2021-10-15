/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker_api.hpp"

#include <soralog/util.hpp>
#include <thread>

#include "log/logger.hpp"
#include "offchain/impl/offchain_worker_impl.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {

  OffchainWorkerApiImpl::OffchainWorkerApiImpl(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<void> OffchainWorkerApiImpl::offchain_worker(
      const primitives::BlockHash &block,
      const primitives::BlockHeader &header) {
    auto worker = std::make_shared<offchain::OffchainWorkerImpl>(
        executor_, block, header);

    auto res = offchain::OffchainWorkerImpl::run(std::move(worker));

    return res;
  }

}  // namespace kagome::runtime
