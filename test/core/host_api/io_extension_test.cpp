/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/io_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/runtime/memory_provider_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/memory.hpp"

using namespace kagome::host_api;
using ::testing::Return;

using kagome::runtime::MemoryProviderMock;
using kagome::runtime::TestMemory;
using kagome::runtime::WasmEnum;
using kagome::runtime::WasmLogLevel;

/**
 * It is impossible to test the console output, but at least we can check, that
 * methods do not fail
 */
class IOExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers(max_log_level_);
  }

  void SetUp() override {
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(std::ref(memory_.memory)));
    io_extension_ = std::make_shared<IOExtension>(memory_provider_);
  }

 protected:
  static constexpr auto max_log_level_ = kagome::log::Level::ERROR;

  TestMemory memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<IOExtension> io_extension_;
};

/**
 * @given io_extension
 * @when try to print string 0123456789abcdef using ext_print_hex
 * @then hex encoded for given string is printed
 */
TEST_F(IOExtensionTest, PrintMessage) {
  auto span = memory_["test"_v];
  io_extension_->ext_logging_log_version_1(
      static_cast<WasmEnum>(WasmLogLevel::Error), span, span);
}

/**
 * @given io_extension
 * @when try to get max log level
 * @then log level returned
 * @note somehow HostApi log level is OFF
 */
TEST_F(IOExtensionTest, GetMaxLogLevel) {
  kagome::log::setLevelOfGroup(kagome::log::defaultGroupName, max_log_level_);
  auto res = io_extension_->ext_logging_max_level_version_1();
  ASSERT_EQ(res, static_cast<WasmEnum>(WasmLogLevel::Error));
  kagome::log::setLevelOfGroup(kagome::log::defaultGroupName,
                               kagome::log::Level::INFO);
}
