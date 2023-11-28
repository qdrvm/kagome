/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "parachain/pvf/pvf_impl.hpp"

#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/module_factory_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "mock/core/runtime/runtime_context_factory_mock.hpp"
#include "mock/core/runtime/runtime_properties_cache_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "parachain/types.hpp"
#include "runtime/executor.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::common::Hash256;
using kagome::parachain::ParachainRuntime;
using kagome::parachain::Pvf;
using kagome::parachain::PvfImpl;
namespace application = kagome::application;
namespace crypto = kagome::crypto;
namespace runtime = kagome::runtime;
namespace blockchain = kagome::blockchain;
namespace primitives = kagome::primitives;
namespace parachain = kagome::parachain;

using namespace kagome::common::literals;

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;

class PvfTest : public testing::Test {
 public:
  void SetUp() {
    testutil::prepareLoggers();
    hasher_ = std::make_shared<testing::NiceMock<crypto::HasherMock>>();
    EXPECT_CALL(*hasher_, blake2b_256(_)).WillRepeatedly(Return(""_hash256));

    module_factory_ = std::make_shared<runtime::ModuleFactoryMock>();
    auto runtime_properties_cache =
        std::make_shared<runtime::RuntimePropertiesCacheMock>();
    auto block_tree = std::make_shared<blockchain::BlockTreeMock>();
    auto sr25519_provider = std::make_shared<crypto::Sr25519ProviderMock>();
    auto parachain_api = std::make_shared<runtime::ParachainHostMock>();

    ON_CALL(*sr25519_provider, verify(_, _, _)).WillByDefault(Return(true));
    ON_CALL(*block_tree, getBlockHeader(_))
        .WillByDefault(Return(primitives::BlockHeader{}));

    ctx_factory = std::make_shared<runtime::RuntimeContextFactoryMock>();
    auto cache = std::make_shared<runtime::RuntimePropertiesCacheMock>();

    auto executor = std::make_shared<runtime::Executor>(ctx_factory, cache);
    kagome::parachain::ValidationResult res;

    EXPECT_CALL(*parachain_api, check_validation_outputs(_, _, _))
        .WillRepeatedly(Return(outcome::success(true)));
    EXPECT_CALL(*parachain_api, session_index_for_child(_))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*parachain_api, session_executor_params(_, _))
        .WillRepeatedly(Return(outcome::success(std::nullopt)));

    auto app_state_manager =
        std::make_shared<application::AppStateManagerMock>();

    pvf_ = std::make_shared<PvfImpl>(
        PvfImpl::Config{
            .precompile_modules = false,
            .runtime_instance_cache_size = 2,
            .precompile_threads_num = 0,
        },
        hasher_,
        module_factory_,
        runtime_properties_cache,
        block_tree,
        sr25519_provider,
        parachain_api,
        executor,
        ctx_factory,
        app_state_manager);
  }

  outcome::result<std::shared_ptr<runtime::Module>, runtime::CompilationError>
  make_module_mock(BufferView code, const Hash256 &code_hash) {
    auto module = std::make_shared<runtime::ModuleMock>();
    auto instance = std::make_shared<runtime::ModuleInstanceMock>();
    ON_CALL(*module, instantiate()).WillByDefault(Return(instance));
    ON_CALL(*instance, getCodeHash()).WillByDefault(ReturnRef(code_hash));
    ON_CALL(*ctx_factory, ephemeral(_, _, _))
        .WillByDefault(Invoke([instance]() {
          return runtime::RuntimeContext::create_TEST(instance);
        }));
    return module;
  }

 protected:
  std::shared_ptr<PvfImpl> pvf_;
  std::shared_ptr<crypto::HasherMock> hasher_;
  std::shared_ptr<runtime::ModuleFactoryMock> module_factory_;
  std::shared_ptr<runtime::RuntimeContextFactoryMock> ctx_factory;
};

Pvf::CandidateReceipt makeReceipt(parachain::ParachainId id,
                                  const Hash256 &code_hash) {
  Pvf::CandidateReceipt receipt{};
  receipt.descriptor.validation_code_hash = code_hash;
  receipt.descriptor.para_id = id;
  return receipt;
}

TEST_F(PvfTest, InstancesCached) {
  Pvf::PersistedValidationData pvf_validation_data{
      .parent_head = Buffer{},
      .relay_parent_number = 42,
      .relay_parent_storage_root = "root"_hash256,
      .max_pov_size = 100,
  };
  Pvf::ParachainBlock para_block{};
  ParachainRuntime code1 = "code1"_buf;
  auto code_hash_1 = "code_hash_1"_hash256;
  EXPECT_CALL(*module_factory_, make(code1.view()))
      .WillOnce(Invoke([&code_hash_1, this](auto code) {
        return make_module_mock(code, code_hash_1);
      }));
  EXPECT_CALL(*hasher_, blake2b_256(code1.view()))
      .WillRepeatedly(Return(code_hash_1));
  // validate with empty cache, instance with code1 for parachain 0 is cached
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(0, code_hash_1), code1));

  // instance with code1 for parachain 0 is taken from the cache
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(0, code_hash_1), code1));

  ParachainRuntime code2 = "code2"_buf;
  auto code_hash_2 = "code_hash_2"_hash256;
  EXPECT_CALL(*module_factory_, make(code2.view()))
      .WillOnce(Invoke([&code_hash_2, this](auto code) {
        return make_module_mock(code, code_hash_2);
      }));
  EXPECT_CALL(*hasher_, blake2b_256(code2.view()))
      .WillRepeatedly(Return(code_hash_2));
  // instance with code2 for parachain 0 is cached, replacing instance for code1
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(0, code_hash_2), code2));

  // instance with code1 for parachain 1 is cached, limit of 2 instances is
  // reached
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(1, code_hash_1), code1));

  // instance with code1 for parachain 1 is taken from cache
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(1, code_hash_1), code1));

  // instance with code2 for parachain 0 is taken from cache
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(0, code_hash_2), code2));

  // instance with code1 for parachain 2 is cached, replacing instance of the
  // least recently used parachain 1
  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(2, code_hash_1), code1));

  ASSERT_OUTCOME_SUCCESS_TRY(pvf_->pvfValidate(
      pvf_validation_data, para_block, makeReceipt(0, code_hash_1), code1));
}
