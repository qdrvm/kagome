/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker_api.hpp"

#include "application/app_configuration.hpp"
#include "log/logger.hpp"
#include "offchain/impl/runner.hpp"
#include "offchain/offchain_worker_factory.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  OffchainWorkerApiImpl::OffchainWorkerApiImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<Watchdog> watchdog,
      std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
      std::shared_ptr<offchain::Runner> runner,
      std::shared_ptr<Executor> executor)
      : app_config_(app_config),
        ocw_factory_(std::move(ocw_factory)),
        runner_(std::move(runner)),
        executor_(std::move(executor)) {
    BOOST_ASSERT(ocw_factory_);
    BOOST_ASSERT(runner_);
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

    auto label = fmt::format("#{}", block);

    auto func = [block = std::move(block),
                 header = std::move(header),
                 executor = executor_] {
      auto res = [&]() -> outcome::result<void> {
        OUTCOME_TRY(ctx, executor->ctx().ephemeralAt(block));
        return executor->call<void>(
            ctx, "OffchainWorkerApi_offchain_worker", header);
      }();

      if (res.has_error()) {
        auto log = log::createLogger("OffchainWorkerApi", "offchain");
        SL_ERROR(log,
                 "Can't execute offchain worker for block {}: {}",
                 block,
                 res.error());
      }
    };

    runner_->run([worker = ocw_factory_->make(),
                  label = std::move(label),
                  func = std::move(func)] { worker->run(func, label); });

    return outcome::success();
  }

}  // namespace kagome::runtime
