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

TEST_P(WavmModuleInitTest, DISABLED_SingleModule) {
  auto wasm = GetParam();
  SL_INFO(log_, "Trying {}", wasm);
  auto code_provider = std::make_shared<kagome::runtime::BasicCodeProvider>(
      std::string(kBasePath) + std::string(wasm));
  EXPECT_OUTCOME_TRUE(code, code_provider->getCodeAt({}));
  auto code_hash = hasher_->blake2b_256(*code);
  auto instance =
      instance_pool_->instantiateFromCode(code_hash, [&] { return code; }, {})
          .value();
  EXPECT_OUTCOME_TRUE(
      runtime_context,
      kagome::runtime::RuntimeContextFactory::stateless(instance));
  EXPECT_OUTCOME_TRUE(response,
                      runtime_context.module_instance->callExportFunction(
                          runtime_context, "Core_version", {}));
  auto memory = runtime_context.module_instance->getEnvironment()
                    .memory_provider->getCurrentMemory();
  GTEST_ASSERT_TRUE(memory.has_value());
  EXPECT_OUTCOME_TRUE(cv, scale::decode<kagome::primitives::Version>(response));
  SL_INFO(log_,
          "Module initialized - spec {}-{}, impl {}-{}",
          cv.spec_name,
          cv.spec_version,
          cv.impl_name,
          cv.impl_version);
}

INSTANTIATE_TEST_SUITE_P(SingleModule,
                         WavmModuleInitTest,
                         testing::ValuesIn(kKusamaParachains));
