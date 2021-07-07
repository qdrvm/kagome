/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/misc_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/core_api_provider_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/runtime_environment_factory_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "runtime/wavm/impl/intrinsic_functions.hpp"
#include "runtime/wavm/impl/module.hpp"
#include "scale/scale.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::crypto::HasherMock;
using kagome::host_api::HostApiMock;
using kagome::host_api::MiscExtension;
using kagome::runtime::CoreApiProviderMock;
using kagome::runtime::Memory;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::PtrSize;
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
 * @when initializing misc extension
 * @then ext_chain_id return the chain id
 */
TEST_F(MiscExtensionTest, Init) {
  auto memory_provider = std::make_shared<MemoryProviderMock>();
  auto memory = std::make_shared<MemoryMock>();
  EXPECT_CALL(*memory_provider, getCurrentMemory())
      .WillRepeatedly(Return(boost::optional<std::shared_ptr<Memory>>(memory)));
  auto core_provider = std::make_shared<CoreApiProviderMock>();
  MiscExtension m{
      42, std::make_shared<HasherMock>(), memory_provider, core_provider};
  MiscExtension m2{
      34, std::make_shared<HasherMock>(), memory_provider, core_provider};
}

/**
 * @given a chain id
 * @when initializing misc extension
 * @then ext_chain_id return the chain id
 */
TEST_F(MiscExtensionTest, CoreVersion) {
  PtrSize state_code1{42, 4};
  PtrSize state_code2{46, 5};
  PtrSize res1{51, 4};
  PtrSize res2{55, 4};

  kagome::primitives::Version v1{};
  v1.authoring_version = 42;
  Buffer v1_enc{encode(boost::make_optional(encode(v1).value())).value()};

  kagome::primitives::Version v2{};
  v2.authoring_version = 24;
  Buffer v2_enc{encode(boost::make_optional(encode(v2).value())).value()};

  auto memory_provider = std::make_shared<MemoryProviderMock>();
  auto memory = std::make_shared<MemoryMock>();
  EXPECT_CALL(*memory_provider, getCurrentMemory())
      .WillRepeatedly(Return(boost::optional<std::shared_ptr<Memory>>(memory)));
  auto core_provider = std::make_shared<CoreApiProviderMock>();

  EXPECT_CALL(*core_provider, makeCoreApi(_, _))
      .WillOnce(Invoke([&v1](auto &, auto &code) {
        auto core = std::make_unique<kagome::runtime::CoreMock>();
        EXPECT_CALL(*core, version()).WillOnce(Return(v1));
        kagome::runtime::wavm::pushHostApi(std::make_shared<HostApiMock>());
        return core;
      }));

  using namespace std::placeholders;

  EXPECT_CALL(*memory, storeBuffer(gsl::span<const uint8_t>(v1_enc)))
      .WillOnce(Return(res1.combine()));
  kagome::host_api::MiscExtension m{
      42, std::make_shared<HasherMock>(), memory_provider, core_provider};
  ASSERT_EQ(m.ext_misc_runtime_version_version_1(state_code1.combine()),
            res1.combine());

  EXPECT_CALL(*core_provider, makeCoreApi(_, _))
      .WillOnce(Invoke([&v2](auto &, auto &code) {
        auto core = std::make_unique<kagome::runtime::CoreMock>();
        EXPECT_CALL(*core, version()).WillOnce(Return(v2));
        kagome::runtime::wavm::pushHostApi(std::make_shared<HostApiMock>());
        return core;
      }));
  EXPECT_CALL(*memory, storeBuffer(gsl::span<const uint8_t>(v2_enc)))
      .WillOnce(Return(res2.combine()));
  kagome::host_api::MiscExtension m2(
      34, std::make_shared<HasherMock>(), memory_provider, core_provider);
  ASSERT_EQ(m2.ext_misc_runtime_version_version_1(state_code2.combine()),
            res2.combine());
}
