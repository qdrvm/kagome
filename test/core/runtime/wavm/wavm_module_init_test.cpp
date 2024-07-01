/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/runtime/wavm/wavm_runtime_test.hpp"

#include "core/runtime/wavm/runtime_paths.hpp"
#include "testutil/prepare_loggers.hpp"

class WavmModuleInitTest : public ::testing::TestWithParam<std::string_view>,
                           public WavmRuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    SetUpImpl();
  }

  kagome::log::Logger log_ = kagome::log::createLogger("Test");
};

INSTANTIATE_TEST_SUITE_P(SingleModule,
                         WavmModuleInitTest,
                         testing::ValuesIn(kKusamaParachains));
