/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/offchain_worker_api.hpp"

namespace kagome {
  class Watchdog;
}  // namespace kagome
namespace kagome::application {
  class AppConfiguration;
}
namespace kagome::offchain {
  class OffchainWorkerFactory;
  class Runner;
}  // namespace kagome::offchain

namespace kagome::runtime {

  class Executor;

  outcome::result<void> callOffchainWorkerApi(
      Executor &executor,
      const primitives::BlockHash &block,
      const primitives::BlockHeader &header);

  class OffchainWorkerApiImpl final : public OffchainWorkerApi {
   public:
    OffchainWorkerApiImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<Watchdog> watchdog,
        std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
        std::shared_ptr<Executor> executor);

    outcome::result<void> offchain_worker(
        const primitives::BlockHash &block,
        const primitives::BlockHeader &header) override;

   private:
    const application::AppConfiguration &app_config_;
    std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory_;
    std::shared_ptr<offchain::Runner> runner_;
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime
