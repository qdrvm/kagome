/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/misc_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/runtime/core_factory_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/runtime_environment_factory_mock.hpp"
#include "mock/core/runtime/wasm_memory_mock.hpp"
#include "scale/scale.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::host_api::MiscExtension;
using kagome::runtime::WasmMemoryMock;
using kagome::runtime::WasmResult;
using kagome::runtime::binaryen::CoreFactoryMock;
using kagome::runtime::binaryen::RuntimeEnvironmentFactoryMock;
using kagome::scale::encode;
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
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST_F(MiscExtensionTest, Init) {
  auto core_factory_mock =
      std::make_shared<testing::NiceMock<CoreFactoryMock>>();
  auto runtime_env_factory_mock =
      std::make_shared<testing::NiceMock<RuntimeEnvironmentFactoryMock>>();
  auto memory = std::make_shared<testing::NiceMock<WasmMemoryMock>>();
  MiscExtension m{42, core_factory_mock, runtime_env_factory_mock, memory};
  ASSERT_EQ(m.ext_chain_id(), 42);

  MiscExtension m2{34, core_factory_mock, runtime_env_factory_mock, memory};
  ASSERT_EQ(m2.ext_chain_id(), 34);
}

/**
 * @given a chain id
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST_F(MiscExtensionTest, CoreVersion) {
  auto memory = std::make_shared<testing::NiceMock<WasmMemoryMock>>();
  WasmResult state_code1{42, 4};
  WasmResult state_code2{46, 5};
  WasmResult res1{51, 4};
  WasmResult res2{55, 4};

  kagome::primitives::Version v1{};
  v1.authoring_version = 42;
  Buffer v1_enc{encode(boost::make_optional(encode(v1).value())).value()};

  kagome::primitives::Version v2{};
  v2.authoring_version = 24;
  Buffer v2_enc{encode(boost::make_optional(encode(v2).value())).value()};

  auto core_factory_mock = std::make_shared<CoreFactoryMock>();
  EXPECT_CALL(*core_factory_mock, createWithCode(_, _))
      .WillOnce(Invoke([&v1](auto &env, auto &provider) {
        auto core = std::make_unique<kagome::runtime::CoreMock>();
        EXPECT_CALL(*core, version(_)).WillOnce(Return(v1));
        return core;
      }));
  auto runtime_env_factory_mock =
      std::make_shared<testing::NiceMock<RuntimeEnvironmentFactoryMock>>();

  using namespace std::placeholders;

  EXPECT_CALL(*memory, storeBuffer(gsl::span<const uint8_t>(v1_enc)))
      .WillOnce(Return(res1.combine()));
  kagome::host_api::MiscExtension m{
      42, core_factory_mock, runtime_env_factory_mock, memory};
  ASSERT_EQ(m.ext_misc_runtime_version_version_1(state_code1.combine()), res1);

  EXPECT_CALL(*core_factory_mock, createWithCode(_, _))
      .WillOnce(Invoke([&v2](auto &env, auto &provider) {
        auto core = std::make_unique<kagome::runtime::CoreMock>();
        EXPECT_CALL(*core, version(_)).WillOnce(Return(v2));
        return core;
      }));
  EXPECT_CALL(*memory, storeBuffer(gsl::span<const uint8_t>(v2_enc)))
      .WillOnce(Return(res2.combine()));
  kagome::host_api::MiscExtension m2(
      34, core_factory_mock, runtime_env_factory_mock, memory);
  ASSERT_EQ(m2.ext_misc_runtime_version_version_1(state_code2.combine()), res2);
}
