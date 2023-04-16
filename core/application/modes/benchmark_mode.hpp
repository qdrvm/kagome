/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BENCHMARK_MODE_HPP
#define KAGOME_BENCHMARK_MODE_HPP

#include "application/mode.hpp"

#include <memory>

#include "log/logger.hpp"

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::benchmark {
  class BlockExecutionBenchmark;
}

namespace kagome::application::mode {

  class BenchmarkMode : public Mode {
   public:
    BenchmarkMode(
        std::shared_ptr<AppConfiguration> app_config,
        std::shared_ptr<benchmark::BlockExecutionBenchmark> block_benchmark);

    virtual int run() const override;

   private:
    std::shared_ptr<AppConfiguration> app_config_;
    std::shared_ptr<benchmark::BlockExecutionBenchmark> block_benchmark_;
    log::Logger logger_ = log::createLogger("BenchmarkMode", "application");
  };

}  // namespace kagome::application::mode

#endif  // KAGOME_BENCHMARK_MODE_HPP
