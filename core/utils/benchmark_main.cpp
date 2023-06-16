/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_configuration_impl.hpp"
#include "benchmark/block_execution_benchmark.hpp"
#include "injector/application_injector.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "profiler/profiler.hpp"

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

    std::cout << "-------------------------------" << std::endl;

    iroha::performance_tools::prepareThreadReport();
    std::unique_ptr<iroha::performance_tools::IReportViewer> viewer;
    iroha::performance_tools::getThreadReport(viewer);
    std::string report;
    if (auto &thread_viewer = viewer->getThreadIterator(); thread_viewer.threadFirst()) {
      do {
        auto &m_it = thread_viewer.getMethodIterator();
        auto &s_it = thread_viewer.getStackIterator();

        m_it.sortMethods(iroha::performance_tools::SortType::kSortByCounter, false);
        m_it.printMethods(report);

        s_it.sortStacks(iroha::performance_tools::SortType::kSortByCounter, false);
        s_it.printStacks(report);
      } while (thread_viewer.threadNext());
    }
    std::cout << report << std::endl;
    return 0;
  }

}  // namespace kagome