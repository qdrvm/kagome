/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <qtils/test/outcome.hpp>

#include <algorithm>
#include <random>
#include <ranges>

#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

#include "runtime/common/runtime_instances_pool.hpp"

#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/runtime/instrument_wasm.hpp"
#include "mock/core/runtime/module_factory_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_mock.hpp"

using kagome::application::AppConfigurationMock;
using kagome::common::Buffer;
using kagome::runtime::ModuleFactoryMock;
using kagome::runtime::ModuleInstanceMock;
using kagome::runtime::ModuleMock;
using kagome::runtime::NoopWasmInstrumenter;
using kagome::runtime::RuntimeContext;
using kagome::runtime::RuntimeInstancesPool;
using kagome::runtime::RuntimeInstancesPoolImpl;
using testing::_;
using testing::Return;

RuntimeInstancesPool::CodeHash make_code_hash(int i) {
  return RuntimeInstancesPool::CodeHash::fromString(
             fmt::format("{: >32}", fmt::format("code_hash_{:0>3}", i)))
      .value();
}

TEST(InstancePoolTest, HeavilyMultithreadedCompilation) {
  testutil::prepareLoggers();

  using namespace std::chrono_literals;

  auto module_instance_mock = std::make_shared<ModuleInstanceMock>();

  auto module_mock = std::make_shared<ModuleMock>();
  EXPECT_CALL(*module_mock, instantiate())
      .WillRepeatedly(testing::Return(module_instance_mock));

  auto module_factory = std::make_shared<ModuleFactoryMock>();
  auto code = std::make_shared<Buffer>("runtime_code"_buf);

  static constexpr int THREAD_NUM = 100;
  static constexpr int POOL_SIZE = 10;

  AppConfigurationMock app_config;
  EXPECT_CALL(app_config, runtimeCacheDirPath()).WillRepeatedly(Return("/tmp"));
  auto pool = std::make_shared<RuntimeInstancesPoolImpl>(
      app_config,
      module_factory,
      std::make_shared<NoopWasmInstrumenter>(),
      POOL_SIZE);

  EXPECT_CALL(*module_factory, compilerType())
      .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(*module_factory, compile(_, _))
      .Times(POOL_SIZE)
      .WillRepeatedly([&] {
        std::this_thread::sleep_for(1s);
        return outcome::success();
      });
  EXPECT_CALL(*module_factory, loadCompiled(_))
      .Times(POOL_SIZE)
      .WillRepeatedly([&] {
        std::this_thread::sleep_for(1s);
        return module_mock;
      });

  std::vector<std::thread> threads;
  for (int i = 0; i < THREAD_NUM; i++) {
    threads.emplace_back([&pool, &code, i]() {
      EXPECT_OK(pool->instantiateFromCode(
          make_code_hash(i % POOL_SIZE), [&] { return code; }, {}));
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  testing::Mock::VerifyAndClearExpectations(pool.get());

  // check that all POOL_SIZE instances are in cache
  for (int i = 0; i < POOL_SIZE; i++) {
    EXPECT_OK(pool->instantiateFromCode(
        make_code_hash(i),
        []() -> decltype(code) { throw std::logic_error{"already compiled"}; },
        {}));
  }
}
