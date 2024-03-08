/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/misc_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/core_api_factory_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/memory.hpp"

using kagome::common::Buffer;
using kagome::crypto::HasherMock;
using kagome::host_api::MiscExtension;
using kagome::runtime::CoreApiFactoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::TestMemory;
using scale::encode;
using testing::_;
using testing::Invoke;
using testing::Return;

class MiscExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }
};

/**
 * @given a chain id
 * @when initializing misc extension
 * @then ext_chain_id return the chain id
 */
TEST_F(MiscExtensionTest, CoreVersion) {
  kagome::primitives::Version v1{};
  v1.authoring_version = 42;
  Buffer v1_enc{encode(std::make_optional(encode(v1).value())).value()};

  auto memory_provider = std::make_shared<MemoryProviderMock>();
  TestMemory memory;
  EXPECT_CALL(*memory_provider, getCurrentMemory())
      .WillRepeatedly(Return(std::ref(memory)));
  auto core_factory = std::make_shared<CoreApiFactoryMock>();

  EXPECT_CALL(*core_factory, make(_, _))
      .WillOnce(Invoke([v1](auto &&, auto &&) {
        auto core = std::make_unique<kagome::runtime::CoreMock>();
        EXPECT_CALL(*core, version()).WillOnce(Return(v1));
        return core;
      }));

  kagome::host_api::MiscExtension m{
      42, std::make_shared<HasherMock>(), memory_provider, core_factory};
  ASSERT_EQ(memory[m.ext_misc_runtime_version_version_1(memory["test"_v])],
            v1_enc);
}
