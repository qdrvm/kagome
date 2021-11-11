/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker_api.hpp"

#include "offchain/offchain_worker_factory.hpp"

namespace kagome::runtime {

  OffchainWorkerApiImpl::OffchainWorkerApiImpl(
      std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
      std::shared_ptr<Executor> executor)
      : ocw_factory_(std::move(ocw_factory)), executor_(std::move(executor)) {
    BOOST_ASSERT(ocw_factory_);
    BOOST_ASSERT(executor_);
  }

  outcome::result<void> OffchainWorkerApiImpl::offchain_worker(
      const primitives::BlockHash &block,
      const primitives::BlockHeader &header) {
    auto worker = ocw_factory_->make(executor_, header);

    auto res = worker->run();

    return res;
  }

}  // namespace kagome::runtime
