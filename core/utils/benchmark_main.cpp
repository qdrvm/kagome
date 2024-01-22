/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_configuration_impl.hpp"
#include "benchmark/block_execution_benchmark.hpp"
#include "common/visitor.hpp"
#include "injector/application_injector.hpp"
#include "runtime/runtime_api/impl/core.hpp"

namespace kagome {

  int benchmark_main(int argc, const char **argv) {
    auto logger = kagome::log::createLogger("Configuration",
                                            kagome::log::defaultGroupName);
    if (argc == 1) {
      SL_ERROR(logger,
               "Usage: kagome benchmark BENCHMARK-TYPE BENCHMARK-OPTIONS\n"
               "Available benchmark types are: block");
      return -1;
    }

    auto app_config = std::make_shared<application::AppConfigurationImpl>();
    if (!app_config->initializeFromArgs(argc, argv)) {
      SL_ERROR(logger, "Failed to initialize kagome!");
      return -1;
    }
    kagome::log::tuneLoggingSystem(app_config->log());

    injector::KagomeNodeInjector injector{app_config};

    auto config_opt = app_config->getBenchmarkConfig();
    if (!config_opt) {
      SL_ERROR(logger, "CLI params for benchmark are missing!");
      return -1;
    }
    auto &benchmark_config = *config_opt;

    auto block_benchmark = injector.injectBlockBenchmark();

    auto res = visit_in_place(
        benchmark_config,
        [&](application::BlockBenchmarkConfig config) -> outcome::result<void> {
          benchmark::BlockExecutionBenchmark::Config config_{
              .start = config.from,
              .end = config.to,
              .times = config.times,
          };

          SL_INFO(logger,
                  "Kagome started. Version: {} ",
                  app_config->nodeVersion());

          OUTCOME_TRY(block_benchmark->run(config_));

          return outcome::success();
        });

    if (res.has_error()) {
      SL_ERROR(logger, "Failed to run benchmark: {}", res.error().message());
      logger->flush();

      return res.error().value();
    }

    SL_INFO(logger, "Kagome benchmark stopped");
    logger->flush();

    return 0;
  }

}  // namespace kagome
