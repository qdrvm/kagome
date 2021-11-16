/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker_api.hpp"

#include "application/app_configuration.hpp"
#include "offchain/offchain_worker_factory.hpp"

namespace kagome::runtime {

  OffchainWorkerApiImpl::OffchainWorkerApiImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
      std::shared_ptr<Executor> executor)
      : app_config_(app_config),
        ocw_factory_(std::move(ocw_factory)),
        executor_(std::move(executor)) {
    BOOST_ASSERT(ocw_factory_);
    BOOST_ASSERT(executor_);
  }

  outcome::result<void> OffchainWorkerApiImpl::offchain_worker(
      const primitives::BlockHash &block,
      const primitives::BlockHeader &header) {
    // Never start offchain workers
    if (app_config_.offchainWorkerMode()
        == application::AppConfiguration::OffchainWorkerMode::Never) {
      return outcome::success();
    }

    // Not start offchain workers if node is not validating,
    // but offchain workers runs when node is validating
    if (app_config_.offchainWorkerMode()
        == application::AppConfiguration::OffchainWorkerMode::WhenValidating) {
      if (app_config_.roles().flags.authority != 1) {
        return outcome::success();
      }
    }

    auto worker = ocw_factory_->make(executor_, header);

    auto res = worker->run();

    return res;
  }

}  // namespace kagome::runtime
