/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/executor.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/module_repository_mock.hpp"
#include "mock/core/runtime/runtime_environment_factory_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::host_api::HostApiMock;
using kagome::runtime::BasicCodeProvider;
using kagome::runtime::Executor;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::ModuleRepository;
using kagome::runtime::ModuleRepositoryMock;
using kagome::runtime::TrieStorageProviderMock;

class ExecutorTest: public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }
};

TEST_F(ExecutorTest, LatestStateSwitchesCorrectly) {
  auto storage_provider = std::make_shared<TrieStorageProviderMock>();
  auto host_api = std::make_shared<HostApiMock>();
  auto memory_provider = std::make_shared<MemoryProviderMock>();
  auto code_provider = std::make_shared<BasicCodeProvider>(
      boost::filesystem::path(__FILE__).parent_path().string()
      + "/wasm/sumtwo.wasm");
  auto module_repo = std::make_shared<ModuleRepositoryMock>();
  auto header_repo = std::make_shared<BlockHeaderRepositoryMock>();

  auto env_factory =
      std::make_shared<kagome::runtime::RuntimeEnvironmentFactoryMock>(
          storage_provider,
          host_api,
          memory_provider,
          code_provider,
          module_repo,
          header_repo);
  Executor executor{header_repo, env_factory};
  executor.persistentCallAt<int>({42, "hash1"_hash256}, "addTwo", 2, 3);
}
