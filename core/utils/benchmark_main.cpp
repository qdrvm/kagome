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
    log::Logger config_logger = log::createLogger("Configuration");
    if (argc == 1) {
      SL_ERROR(config_logger,
               "Usage: kagome benchmark BENCHMARK-TYPE BENCHMARK-OPTIONS\n"
               "Available benchmark types are: block");
      return -1;
    }

    auto config =
        std::make_shared<application::AppConfigurationImpl>(config_logger);
    if (!config->initializeFromArgs(argc, argv)) {
      SL_ERROR(config_logger, "Failed to initialize kagome!");
      return -1;
    }
    kagome::log::tuneLoggingSystem(config->log());

    injector::KagomeNodeInjector injector{config};

    auto config_opt = config->getBenchmarkConfig();
    if (!config_opt) {
      SL_ERROR(config_logger, "CLI params for benchmark are missing!");
      return -1;
    }
    auto &benchmark_config = *config_opt;

    auto block_benchmark = injector.injectBlockBenchmark();

    auto res = visit_in_place(
        benchmark_config,
        [&block_benchmark](
            application::BlockBenchmarkConfig config) -> outcome::result<void> {
          benchmark::BlockExecutionBenchmark::Config config_{
              .start = config.from,
              .end = config.to,
              .times = config.times,
          };
          OUTCOME_TRY(block_benchmark->run(config_));

          return outcome::success();
        });

    if (res.has_error()) {
      SL_ERROR(
          config_logger, "Failed to run benchmark: {}", res.error().message());
      return res.error().value();
    }

    return 0;
  }

}  // namespace kagome
