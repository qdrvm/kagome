/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "mock/core/runtime/module_factory_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_mock.hpp"
#include "parachain/pvf/pvf_runtime_cache.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace parachain = kagome::parachain;
namespace runtime = kagome::runtime;
using namespace kagome::common::literals;
using testing::Invoke;
using testing::Return;
using testing::ReturnRefOfCopy;

class PvfRuntimeCacheTest : public testing::Test {
 public:
  void SetUp() {
    module_factory_mock =
        std::make_shared<kagome::runtime::ModuleFactoryMock>();
    cache =
        std::make_unique<parachain::PvfRuntimeCache>(module_factory_mock, 2);
  }

 protected:
  std::unique_ptr<parachain::PvfRuntimeCache> cache;
  std::shared_ptr<kagome::runtime::ModuleFactoryMock> module_factory_mock;
};

std::unique_ptr<runtime::ModuleMock> make_module_mock(
    kagome::common::BufferView code) {
  auto module = std::make_unique<runtime::ModuleMock>();
  auto instance = std::make_shared<runtime::ModuleInstanceMock>();
  kagome::common::Hash256 hash;
  std::copy_n(code.data(), code.size(), hash.rbegin());
  ON_CALL(*instance, getCodeHash()).WillByDefault(ReturnRefOfCopy(hash));

  ON_CALL(*module, instantiate()).WillByDefault(Return(instance));
  return module;
}

TEST_F(PvfRuntimeCacheTest, BasicScenario) {
  EXPECT_CALL(*module_factory_mock, make("code1"_buf.view()))
      .WillOnce(Invoke(make_module_mock));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(0, "code1"_hash256, "code1"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(0, "code1"_hash256, "code1"_buf));

  EXPECT_CALL(*module_factory_mock, make("code2"_buf.view()))
      .WillOnce(Invoke(make_module_mock));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(0, "code2"_hash256, "code2"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(0, "code2"_hash256, "code2"_buf));

  EXPECT_CALL(*module_factory_mock, make("code1"_buf.view()))
      .WillOnce(Invoke(make_module_mock));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(1, "code1"_hash256, "code1"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(1, "code1"_hash256, "code1"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(0, "code2"_hash256, "code2"_buf));

  EXPECT_CALL(*module_factory_mock, make("code1"_buf.view()))
      .WillOnce(Invoke(make_module_mock));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(2, "code1"_hash256, "code1"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(0, "code2"_hash256, "code2"_buf));

  EXPECT_CALL(*module_factory_mock, make("code1"_buf.view()))
      .WillOnce(Invoke(make_module_mock));
  ASSERT_OUTCOME_SUCCESS_TRY(
      cache->requestInstance(1, "code1"_hash256, "code1"_buf));
}
