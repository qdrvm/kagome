/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/modes/benchmark_mode.hpp"

#include <boost/assert.hpp>
#include <benchmark/benchmark.h>

#include "application/app_configuration.hpp"
#include "benchmark/block_execution_benchmark.hpp"
#include "common/visitor.hpp"

namespace kagome::application::mode {

  BenchmarkMode::BenchmarkMode(
      std::shared_ptr<AppConfiguration> app_config,
      std::shared_ptr<benchmark::BlockExecutionBenchmark> block_benchmark)
      : app_config_{app_config}, block_benchmark_{block_benchmark} {
    BOOST_ASSERT(app_config_ != nullptr);
    BOOST_ASSERT(block_benchmark_ != nullptr);
  }

  int BenchmarkMode::run() const {
    int argc = 0;
    char** argv = nullptr;
    ::benchmark::Initialize(&argc, argv);

    auto config_opt = app_config_->getBenchmarkConfig();
    if (!config_opt) {
      SL_ERROR(logger_, "CLI params for benchmark are missing!");
      return -1;
    }
    auto &config = *config_opt;

    auto res = visit_in_place(
        config, [this](BlockBenchmarkConfig config) -> outcome::result<void> {
          benchmark::BlockExecutionBenchmark::Config config_{
              .start = config.from,
              .end = config.to,
              .times = config.times,
          };
          OUTCOME_TRY(block_benchmark_->run(config_));

          return outcome::success();
        });

    if (res.has_error()) {
      SL_ERROR(logger_, "Failed to run benchmark: {}", res.error().message());
      return res.error().value();
    }

    ::benchmark::Shutdown();
    return 0;
  }

}  // namespace kagome::application::mode
