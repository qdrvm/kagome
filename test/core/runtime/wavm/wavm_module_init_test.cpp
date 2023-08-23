/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

using kagome::runtime::BasicCodeProvider;

class WavmModuleInitTest : public ::testing::Test {
 public:
  void SetUp() override {
    code_provider_ = std::make_shared<BasicCodeProvider>(
        "/Users/igor/Downloads/Telegram "
        "Desktop/kusama-para-code/"
        "0f72fb8e0ef96652558a34f3993e22be0c6f0c50ab267ad597f9165cf3e8909b."
        "wasm");
  }

  std::shared_ptr<BasicCodeProvider> code_provider_;
};

TEST_F(WavmModuleInitTest, SingleModule) {
  EXPECT_OUTCOME_TRUE(code, code_provider_->getCodeAt({}));

}
