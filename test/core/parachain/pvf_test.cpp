/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "parachain/pvf/pvf_impl.hpp"

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/module_factory_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "mock/core/runtime/runtime_properties_cache_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::parachain::ParachainRuntime;
using kagome::parachain::Pvf;
using kagome::parachain::PvfImpl;
namespace crypto = kagome::crypto;
namespace runtime = kagome::runtime;
namespace blockchain = kagome::blockchain;
namespace primitives = kagome::primitives;

using namespace kagome::common::literals;

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;

class PvfTest : public testing::Test {
 public:
  void SetUp() {
    testutil::prepareLoggers();
    auto hasher = std::make_shared<crypto::HasherMock>();
    module_factory_ = std::make_shared<runtime::ModuleFactoryMock>();
    auto runtime_properties_cache =
        std::make_shared<runtime::RuntimePropertiesCacheMock>();
    auto block_header_repository =
        std::make_shared<blockchain::BlockHeaderRepositoryMock>();
    auto sr25519_provider = std::make_shared<crypto::Sr25519ProviderMock>();
    auto parachain_api = std::make_shared<runtime::ParachainHostMock>();
    PvfImpl::Config config{
        .instance_cache_size = 2,
    };

    ON_CALL(*sr25519_provider, verify(_, _, _)).WillByDefault(Return(true));
    ON_CALL(*block_header_repository, getHashByNumber(_))
        .WillByDefault(Return("hash"_hash256));
    ON_CALL(*block_header_repository, getBlockHeader(_))
        .WillByDefault(Return(primitives::BlockHeader{}));

    pvf_ = std::make_shared<PvfImpl>(hasher,
                                     module_factory_,
                                     runtime_properties_cache,
                                     block_header_repository,
                                     sr25519_provider,
                                     parachain_api,
                                     config);
  }

 protected:
  std::shared_ptr<PvfImpl> pvf_;
  std::shared_ptr<runtime::ModuleFactoryMock> module_factory_;
};

TEST_F(PvfTest, InstancesCached) {
  Pvf::PersistedValidationData pvf_validation_data{
      .parent_head = Buffer{},
      .relay_parent_number = 42,
      .relay_parent_storage_root = "root"_hash256,
      .max_pov_size = 100,
  };
  Pvf::ParachainBlock para_block;
  Pvf::CandidateReceipt receipt;
  ParachainRuntime code = "code1"_buf;
  EXPECT_CALL(*module_factory_, make(code.view()))
      .WillOnce(
          Invoke([](gsl::span<const uint8_t> code)
                     -> outcome::result<std::unique_ptr<runtime::Module>> {
            auto module = std::make_unique<runtime::ModuleMock>();
            auto instance = std::make_shared<runtime::ModuleInstanceMock>();
            ON_CALL(*module, instantiate()).WillByDefault(Return(instance));

            auto memory_provider =
                std::make_shared<runtime::MemoryProviderMock>();
            auto storage_provider =
                std::make_shared<runtime::TrieStorageProviderMock>();
            ON_CALL(*storage_provider, setToEphemeralAt(_))
                .WillByDefault(Return(outcome::success()));
            auto host_api = std::make_shared<kagome::host_api::HostApiMock>();
            static runtime::InstanceEnvironment env{
                memory_provider,
                storage_provider,
                host_api,
                [](runtime::InstanceEnvironment &) {}};
            ON_CALL(*instance, getEnvironment()).WillByDefault(ReturnRef(env));
            ON_CALL(*instance, getGlobal("__heap_base"))
                .WillByDefault(Return(42));
            ON_CALL(*instance, resetMemory(_))
                .WillByDefault(Return(outcome::success()));
            ON_CALL(*instance, callExportFunction("validate_block", _))
                .WillByDefault(Return(runtime::PtrSize(0, 0)));
            ON_CALL(*instance, resetEnvironment())
                .WillByDefault(Return(outcome::success()));
            return module;
          }));
  ASSERT_OUTCOME_SUCCESS_TRY(
      pvf_->pvfValidate(pvf_validation_data, para_block, receipt, code));
}
