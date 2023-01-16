/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/executor.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_repository_mock.hpp"
#include "mock/core/runtime/runtime_environment_factory_mock.hpp"
#include "mock/core/runtime/runtime_properties_cache_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::host_api::HostApiMock;
using kagome::runtime::BasicCodeProvider;
using kagome::runtime::Executor;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::ModuleInstanceMock;
using kagome::runtime::ModuleRepository;
using kagome::runtime::ModuleRepositoryMock;
using kagome::runtime::PtrSize;
using kagome::runtime::RuntimeEnvironment;
using kagome::runtime::RuntimeEnvironmentTemplateMock;
using kagome::runtime::RuntimePropertiesCacheMock;
using kagome::runtime::TrieStorageProviderMock;
using kagome::storage::trie::PersistentTrieBatch;
using kagome::storage::trie::PersistentTrieBatchMock;
using testing::_;
using testing::ElementsAreArray;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;

class ExecutorTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    memory_ = std::make_unique<MemoryMock>();

    header_repo_ = std::make_shared<BlockHeaderRepositoryMock>();

    auto code_provider = std::make_shared<BasicCodeProvider>(
        boost::filesystem::path(__FILE__).parent_path().string()
        + "/wasm/sumtwo.wasm");
    auto module_repo = std::make_shared<ModuleRepositoryMock>();
    env_factory_ =
        std::make_shared<kagome::runtime::RuntimeEnvironmentFactoryMock>(
            code_provider, module_repo, header_repo_);

    cache_ = std::make_shared<RuntimePropertiesCacheMock>();
    ON_CALL(*cache_, getVersion(_, _))
        .WillByDefault(
            Invoke([](const auto &hash, auto func) { return func(); }));
    ON_CALL(*cache_, getMetadata(_, _))
        .WillByDefault(
            Invoke([](const auto &hash, auto func) { return func(); }));

    storage_ = std::make_shared<kagome::storage::trie::TrieStorageMock>();
  }

  void preparePersistentCall(
      kagome::primitives::BlockInfo const &blockchain_state,
      kagome::storage::trie::RootHash const &storage_state,
      int arg1,
      int arg2,
      int res) {
    static Buffer enc_args;
    enc_args = Buffer{scale::encode(arg1, arg2).value()};
    const PtrSize RESULT_LOCATION{3, 4};

    Buffer enc_res{scale::encode(res).value()};
    EXPECT_CALL(*memory_, loadN(RESULT_LOCATION.ptr, RESULT_LOCATION.size))
        .WillOnce(Return(enc_res));
    EXPECT_CALL(*env_factory_, start(blockchain_state.hash))
        .WillOnce(Invoke(
            [weak_env_factory =
                 std::weak_ptr<kagome::runtime::RuntimeEnvironmentFactoryMock>{
                     env_factory_},
             blockchain_state,
             storage_state,
             this,
             RESULT_LOCATION](auto &) {
              auto env_template =
                  std::make_unique<RuntimeEnvironmentTemplateMock>(
                      weak_env_factory, blockchain_state, storage_state);
              EXPECT_CALL(*env_template, persistent())
                  .WillOnce(ReturnRef(*env_template));
              EXPECT_CALL(*env_template, make(_))
                  .WillOnce(Invoke([this, RESULT_LOCATION, blockchain_state] {
                    auto module_instance =
                        std::make_shared<ModuleInstanceMock>();
                    EXPECT_CALL(*module_instance, resetEnvironment())
                        .WillOnce(Return(outcome::success()));
                    EXPECT_CALL(*module_instance,
                                callExportFunction(std::string_view{"addTwo"},
                                                   enc_args.view()))
                        .WillOnce(Return(RESULT_LOCATION));
                    auto memory_provider =
                        std::make_shared<kagome::runtime::MemoryProviderMock>();
                    EXPECT_CALL(*memory_provider, getCurrentMemory())
                        .WillOnce(
                            Return(std::optional<std::reference_wrapper<
                                       kagome::runtime::Memory>>(*memory_)));

                    auto storage_provider = std::make_shared<
                        kagome::runtime::TrieStorageProviderMock>();
                    auto batch = std::make_shared<
                        kagome::storage::trie::PersistentTrieBatchMock>();

                    return std::make_unique<RuntimeEnvironment>(
                        module_instance,
                        memory_provider,
                        storage_provider,
                        blockchain_state);
                  }));
              return env_template;
            }));
  }

  void prepareEphemeralCall(
      kagome::primitives::BlockInfo const &blockchain_state,
      kagome::storage::trie::RootHash const &storage_state,
      int arg1,
      int arg2,
      int res) {
    static Buffer enc_args;
    enc_args = Buffer{scale::encode(arg1, arg2).value()};
    const PtrSize RESULT_LOCATION{3, 4};
    Buffer enc_res{scale::encode(res).value()};
    EXPECT_CALL(*memory_, loadN(RESULT_LOCATION.ptr, RESULT_LOCATION.size))
        .WillOnce(Return(enc_res));
    EXPECT_CALL(*env_factory_, start(blockchain_state, storage_state))
        .WillOnce(Invoke(
            [weak_env_factory =
                 std::weak_ptr<kagome::runtime::RuntimeEnvironmentFactoryMock>{
                     env_factory_},
             this,
             RESULT_LOCATION](auto &blockchain_state, auto &storage_state) {
              auto env_template =
                  std::make_unique<RuntimeEnvironmentTemplateMock>(
                      weak_env_factory, blockchain_state, storage_state);
              EXPECT_CALL(*env_template, make(_))
                  .WillOnce(Invoke([this, blockchain_state, RESULT_LOCATION] {
                    auto module_instance =
                        std::make_shared<ModuleInstanceMock>();
                    EXPECT_CALL(*module_instance, resetEnvironment())
                        .WillOnce(Return(outcome::success()));
                    EXPECT_CALL(*module_instance,
                                callExportFunction(std::string_view{"addTwo"},
                                                   enc_args.view()))
                        .WillOnce(Return(RESULT_LOCATION));
                    auto memory_provider =
                        std::make_shared<kagome::runtime::MemoryProviderMock>();
                    EXPECT_CALL(*memory_provider, getCurrentMemory())
                        .WillOnce(
                            Return(std::optional<std::reference_wrapper<
                                       kagome::runtime::Memory>>(*memory_)));

                    auto storage_provider = std::make_shared<
                        kagome::runtime::TrieStorageProviderMock>();

                    return std::make_unique<RuntimeEnvironment>(
                        module_instance,
                        memory_provider,
                        storage_provider,
                        blockchain_state);
                  }));
              return env_template;
            }));
  }

 protected:
  std::unique_ptr<MemoryMock> memory_;
  std::shared_ptr<kagome::runtime::RuntimeEnvironmentFactoryMock> env_factory_;
  std::shared_ptr<kagome::runtime::RuntimePropertiesCacheMock> cache_;
  std::shared_ptr<BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::storage::trie::TrieStorageMock> storage_;
};

TEST_F(ExecutorTest, LatestStateSwitchesCorrectly) {
  Executor executor{env_factory_, cache_};
  kagome::primitives::BlockInfo block_info1{42, "block_hash1"_hash256};
  kagome::primitives::BlockInfo block_info2{43, "block_hash2"_hash256};
  kagome::primitives::BlockInfo block_info3{44, "block_hash3"_hash256};

  preparePersistentCall(block_info1, "state_hash1"_hash256, 2, 3, 5);
  auto env = executor.persistentAt(block_info1.hash).value();
  EXPECT_EQ(executor.call<int>(*env, "addTwo", 2, 3).value(), 5);

  prepareEphemeralCall(block_info1, "state_hash2"_hash256, 7, 10, 17);
  EXPECT_OUTCOME_TRUE(res2,
                      executor.callAt<int>(
                          block_info1, "state_hash2"_hash256, "addTwo", 7, 10));
  ASSERT_EQ(res2, 17);

  preparePersistentCall(block_info1, "state_hash2"_hash256, 0, 0, 0);
  auto env3 = executor.persistentAt(block_info1.hash).value();
  EXPECT_EQ(executor.call<int>(*env3, "addTwo", 0, 0).value(), 0);

  prepareEphemeralCall(block_info1, "state_hash3"_hash256, 7, 10, 17);
  EXPECT_OUTCOME_TRUE(res4,
                      executor.callAt<int>(
                          block_info1, "state_hash3"_hash256, "addTwo", 7, 10));
  ASSERT_EQ(res4, 17);

  preparePersistentCall(block_info2, "state_hash4"_hash256, -5, 5, 0);
  auto env5 = executor.persistentAt(block_info2.hash).value();
  EXPECT_EQ(executor.call<int>(*env5, "addTwo", -5, 5).value(), 0);

  prepareEphemeralCall(block_info2, "state_hash5"_hash256, 7, 10, 17);
  EXPECT_OUTCOME_TRUE(res6,
                      executor.callAt<int>(
                          block_info2, "state_hash5"_hash256, "addTwo", 7, 10));
  ASSERT_EQ(res6, 17);
}
