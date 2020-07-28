/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/misc_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/wasm_memory_mock.hpp"
#include "scale/scale.hpp"

using kagome::common::Buffer;
using kagome::extensions::MiscExtension;
using kagome::runtime::WasmMemoryMock;
using kagome::runtime::WasmResult;
using kagome::scale::encode;
using testing::_;
using testing::Return;

/**
 * @given a chain id
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST(MiscExt, Init) {
  auto memory = std::make_shared<testing::NiceMock<WasmMemoryMock>>();
  MiscExtension m{42, memory, MiscExtension::CoreFactoryMethod{}};
  ASSERT_EQ(m.ext_chain_id(), 42);

  MiscExtension m2{34, memory, MiscExtension::CoreFactoryMethod{}};
  ASSERT_EQ(m2.ext_chain_id(), 34);
}

/**
 * @given a chain id
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST(MiscExt, CoreVersion) {
  auto memory = std::make_shared<testing::NiceMock<WasmMemoryMock>>();
  WasmResult state_code1{42, 4};
  WasmResult state_code2{46, 5};
  WasmResult res1{51, 4};
  WasmResult res2{55, 4};

  kagome::primitives::Version v1{};
  v1.authoring_version = 42;
  Buffer v1_enc{encode(boost::make_optional(v1)).value()};

  kagome::primitives::Version v2{};
  v2.authoring_version = 24;
  Buffer v2_enc{encode(boost::make_optional(v2)).value()};

  auto core_factory_method =
      [&](kagome::primitives::Version v,
          std::shared_ptr<kagome::runtime::WasmProvider>) {
        auto core = std::make_unique<kagome::runtime::CoreMock>();
        EXPECT_CALL(*core, version(_)).WillOnce(Return(v));
        return core;
      };

  using namespace std::placeholders;

  EXPECT_CALL(*memory, storeBuffer(gsl::span<const uint8_t>(v1_enc)))
      .WillOnce(Return(res1.combine()));
  kagome::extensions::MiscExtension m{
      42, memory, std::bind(core_factory_method, v1, _1)};
  ASSERT_EQ(m.ext_misc_runtime_version_version_1(state_code1.combine()), res1);

  EXPECT_CALL(*memory, storeBuffer(gsl::span<const uint8_t>(v2_enc)))
      .WillOnce(Return(res2.combine()));
  kagome::extensions::MiscExtension m2(
      34, memory, std::bind(core_factory_method, v2, _1));
  ASSERT_EQ(m2.ext_misc_runtime_version_version_1(state_code2.combine()), res2);
}
