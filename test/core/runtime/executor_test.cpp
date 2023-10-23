/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/executor.hpp"

#include <gtest/gtest.h>
#include "filesystem/common.hpp"

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_repository_mock.hpp"
#include "mock/core/runtime/runtime_context_factory_mock.hpp"
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
using kagome::runtime::RuntimeContext;
using kagome::runtime::RuntimePropertiesCacheMock;
using kagome::runtime::TrieStorageProviderMock;
using kagome::storage::trie::TrieBatch;
using kagome::storage::trie::TrieBatchMock;
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
        kagome::filesystem::path(__FILE__).parent_path().string()
        + "/wasm/sumtwo.wasm");
    module_repo_ = std::make_shared<ModuleRepositoryMock>();

    cache_ = std::make_shared<RuntimePropertiesCacheMock>();
    ON_CALL(*cache_, getVersion(_, _))
        .WillByDefault(
            Invoke([](const auto &hash, auto func) { return func(); }));
    ON_CALL(*cache_, getMetadata(_, _))
        .WillByDefault(
            Invoke([](const auto &hash, auto func) { return func(); }));
    ON_CALL(*memory_, loadN(_, _))
        .WillByDefault(
            testing::Throw(std::runtime_error{"unexpected memory access"}));

    storage_ = std::make_shared<kagome::storage::trie::TrieStorageMock>();

    ctx_factory_ = std::make_shared<kagome::runtime::RuntimeContextFactoryImpl>(
        module_repo_, header_repo_);
  }

  enum class CallType { Persistent, Ephemeral };

  void prepareCall(const kagome::primitives::BlockInfo &blockchain_state,
                   const kagome::storage::trie::RootHash &storage_state,
                   CallType type,
                   const Buffer &encoded_args,
                   int res) {
    static constexpr PtrSize RESULT_LOCATION{3, 4};

    EXPECT_CALL(*header_repo_, getBlockHeader(blockchain_state.hash))
        .WillRepeatedly(Return(kagome::primitives::BlockHeader{
            blockchain_state.number,  // number
            {},                       // parent
            storage_state,            // state root
            {},                       // extrinsics root
            {}                        // digest
        }));

    auto module_instance = std::make_shared<ModuleInstanceMock>();
    EXPECT_CALL(*module_instance, resetEnvironment())
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*module_instance, resetMemory(_))
        .WillRepeatedly(Return(outcome::success()));
    static const auto code_hash = "code_hash"_hash256;
    EXPECT_CALL(*module_instance, getCodeHash())
        .WillRepeatedly(ReturnRef(code_hash));
    EXPECT_CALL(*module_instance,
                callExportFunction(std::string_view{"addTwo"}, _))
        .WillRepeatedly(Return(RESULT_LOCATION));
    auto memory_provider =
        std::make_shared<kagome::runtime::MemoryProviderMock>();
    EXPECT_CALL(*memory_provider, getCurrentMemory())
        .WillRepeatedly(Return(
            std::optional<std::reference_wrapper<kagome::runtime::Memory>>(
                *memory_)));

    auto storage_provider =
        std::make_shared<kagome::runtime::TrieStorageProviderMock>();
    if (type == CallType::Persistent) {
      EXPECT_CALL(*storage_provider, setToPersistentAt(storage_state, _))
          .WillOnce(Return(outcome::success()));
    } else {
      EXPECT_CALL(*storage_provider, setToEphemeralAt(storage_state))
          .WillOnce(Return(outcome::success()));
    }
    auto env = std::make_shared<kagome::runtime::InstanceEnvironment>(
        memory_provider, storage_provider, nullptr, nullptr);
    EXPECT_CALL(*module_instance, getEnvironment())
        .WillRepeatedly(testing::Invoke(
            [env]() -> const kagome::runtime::InstanceEnvironment & {
              return *env;
            }));
    EXPECT_CALL(*module_repo_, getInstanceAt(blockchain_state, storage_state))
        .WillRepeatedly(testing::Return(module_instance));

    Buffer enc_res{scale::encode(res).value()};
    EXPECT_CALL(*memory_, loadN(RESULT_LOCATION.ptr, RESULT_LOCATION.size))
        .WillOnce(Return(enc_res));
  }

 protected:
  std::unique_ptr<MemoryMock> memory_;
  std::shared_ptr<kagome::runtime::RuntimeContextFactoryImpl> ctx_factory_;
  std::shared_ptr<kagome::runtime::RuntimePropertiesCacheMock> cache_;
  std::shared_ptr<BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::storage::trie::TrieStorageMock> storage_;
  std::shared_ptr<ModuleRepositoryMock> module_repo_;
};

TEST_F(ExecutorTest, LatestStateSwitchesCorrectly) {
  Executor executor{ctx_factory_, cache_};
  kagome::primitives::BlockInfo block_info1{42, "block_hash1"_hash256};
  kagome::primitives::BlockInfo block_info2{43, "block_hash2"_hash256};
  kagome::primitives::BlockInfo block_info3{44, "block_hash3"_hash256};

  Buffer enc_args{scale::encode(2, 3).value()};
  prepareCall(
      block_info1, "state_hash1"_hash256, CallType::Persistent, enc_args, 5);
  auto ctx = ctx_factory_->persistentAt(block_info1.hash, std::nullopt).value();
  auto res = executor.decodedCallWithCtx<int>(ctx, "addTwo", 2, 3).value();
  EXPECT_EQ(res, 5);

  enc_args = scale::encode(7, 10).value();
  prepareCall(
      block_info1, "state_hash2"_hash256, CallType::Ephemeral, enc_args, 17);
  EXPECT_OUTCOME_TRUE(
      res2,
      executor.callAt<int>(
          block_info1.hash, "state_hash2"_hash256, "addTwo", 7, 10));
  ASSERT_EQ(res2, 17);

  enc_args = scale::encode(0, 0).value();
  prepareCall(
      block_info1, "state_hash2"_hash256, CallType::Persistent, enc_args, 0);
  auto ctx3 =
      ctx_factory_->persistentAt(block_info1.hash, std::nullopt).value();
  EXPECT_EQ(executor.decodedCallWithCtx<int>(ctx, "addTwo", 0, 0).value(), 0);

  enc_args = scale::encode(7, 10).value();
  prepareCall(
      block_info1, "state_hash3"_hash256, CallType::Ephemeral, enc_args, 17);
  EXPECT_OUTCOME_TRUE(
      res4,
      executor.callAt<int>(
          block_info1.hash, "state_hash3"_hash256, "addTwo", 7, 10));
  ASSERT_EQ(res4, 17);

  enc_args = scale::encode(-5, 5).value();
  prepareCall(
      block_info2, "state_hash4"_hash256, CallType::Persistent, enc_args, 0);
  auto ctx5 =
      ctx_factory_->persistentAt(block_info2.hash, std::nullopt).value();
  EXPECT_EQ(executor.decodedCallWithCtx<int>(ctx5, "addTwo", -5, 5).value(), 0);

  enc_args = scale::encode(7, 10).value();
  prepareCall(
      block_info2, "state_hash5"_hash256, CallType::Ephemeral, enc_args, 17);
  EXPECT_OUTCOME_TRUE(
      res6,
      executor.callAt<int>(
          block_info2.hash, "state_hash5"_hash256, "addTwo", 7, 10));
  ASSERT_EQ(res6, 17);
}
