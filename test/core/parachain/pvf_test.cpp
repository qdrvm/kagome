/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include <qtils/test/outcome.hpp>

#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/instrument_wasm.hpp"
#include "mock/core/runtime/module_factory_mock.hpp"
#include "mock/core/runtime/module_instance_mock.hpp"
#include "mock/core/runtime/module_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "mock/core/runtime/runtime_context_factory_mock.hpp"
#include "mock/core/runtime/runtime_properties_cache_mock.hpp"
#include "mock/span.hpp"
#include "parachain/pvf/pool.hpp"
#include "parachain/pvf/pvf_impl.hpp"
#include "parachain/pvf/pvf_thread_pool.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "parachain/types.hpp"
#include "runtime/executor.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::TestThreadPool;
using kagome::application::AppConfigurationMock;
using kagome::application::StartApp;
using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::common::Hash256;
using kagome::crypto::HasherImpl;
using kagome::parachain::ParachainId;
using kagome::parachain::ParachainRuntime;
using kagome::parachain::Pvf;
using kagome::parachain::PvfImpl;
using kagome::parachain::PvfPool;
using kagome::parachain::PvfThreadPool;
using kagome::parachain::ValidationResult;
using kagome::runtime::MemoryLimits;
using kagome::runtime::ModuleFactoryMock;
using kagome::runtime::ModuleInstanceMock;
using kagome::runtime::ModuleMock;
using kagome::runtime::NoopWasmInstrumenter;
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
    EXPECT_CALL(*app_config_, usePvfSubprocess()).WillRepeatedly(Return(false));
    EXPECT_CALL(*app_config_, parachainRuntimeInstanceCacheSize())
        .WillRepeatedly(Return(2));

    auto block_tree = std::make_shared<blockchain::BlockTreeMock>();
    auto sr25519_provider = std::make_shared<crypto::Sr25519ProviderMock>();
    auto parachain_api = std::make_shared<runtime::ParachainHostMock>();

    ON_CALL(*sr25519_provider, verify(_, _, _)).WillByDefault(Return(true));
    ON_CALL(*block_tree, getBlockHeader(_))
        .WillByDefault(Return(primitives::BlockHeader{}));

    ctx_factory = std::make_shared<runtime::RuntimeContextFactoryMock>();
    auto cache = std::make_shared<runtime::RuntimePropertiesCacheMock>();

    auto executor = std::make_shared<runtime::Executor>(ctx_factory, cache);

    EXPECT_CALL(*parachain_api, check_validation_outputs(_, _, _))
        .WillRepeatedly(Return(outcome::success(true)));
    EXPECT_CALL(*parachain_api, session_index_for_child(_))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*parachain_api, session_executor_params(_, _))
        .WillRepeatedly(Return(outcome::success(std::nullopt)));

    auto app_state_manager = std::make_shared<StartApp>();

    PvfThreadPool pvf_thread{TestThreadPool{io_}};
    pvf_ = std::make_shared<PvfImpl>(
        PvfImpl::Config{
            .precompile_modules = false,
            .precompile_threads_num = 0,
        },
        nullptr,
        hasher_,
        std::make_shared<PvfPool>(*app_config_,
                                  module_factory_,
                                  std::make_shared<NoopWasmInstrumenter>()),
        block_tree,
        sr25519_provider,
        parachain_api,
        executor,
        ctx_factory,
        pvf_thread,
        app_state_manager,
        app_config_);
    app_state_manager->start();
  }

  auto mockModule(uint8_t code_i) {
    Buffer code{code_i};
    auto code_hash = hasher_->blake2b_256(code);
    EXPECT_CALL(*module_factory_, compilerType())
        .WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(*module_factory_, compile(_, MatchSpan(code), _))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*module_factory_, loadCompiled(_, _)).WillRepeatedly([=] {
      auto module = std::make_shared<ModuleMock>();
      ON_CALL(*module, instantiate()).WillByDefault([=] {
        auto instance = std::make_shared<ModuleInstanceMock>();
        ON_CALL(*instance, callExportFunction(_, "validate_block", _))
            .WillByDefault(
                Return(Buffer{scale::encode(ValidationResult{}).value()}));
        ON_CALL(*instance, getCodeHash()).WillByDefault(Return(code_hash));
        EXPECT_CALL(*instance, stateless())
            .WillRepeatedly(Return(outcome::success()));
        return instance;
      });
      return module;
    });
    return [=, this](ParachainId para) {
      Pvf::PersistedValidationData pvd;
      pvd.max_pov_size = 1;
      Pvf::ParachainBlock pov;
      Pvf::CandidateReceipt receipt;
      receipt.descriptor.validation_code_hash = code_hash;
      receipt.descriptor.para_id = para;
      receipt.descriptor.pov_hash =
          hasher_->blake2b_256(scale::encode(pov).value());
      receipt.descriptor.para_head_hash = hasher_->blake2b_256(pvd.parent_head);
      receipt.commitments_hash = hasher_->blake2b_256(
          scale::encode(Pvf::CandidateCommitments{}).value());
      testing::MockFunction<void(outcome::result<Pvf::Result>)> cb;
      EXPECT_CALL(cb, Call(_)).WillOnce([](outcome::result<Pvf::Result> r) {
        EXPECT_OUTCOME_SUCCESS(r);
      });
      pvf_->pvfValidate(pvd,
                        pov,
                        receipt,
                        code,
                        runtime::PvfExecTimeoutKind::Approval,
                        cb.AsStdFunction());
      io_->restart();
      io_->run();
    };
  }

 protected:
  std::shared_ptr<AppConfigurationMock> app_config_ =
      std::make_shared<AppConfigurationMock>();
  std::shared_ptr<PvfImpl> pvf_;
  std::shared_ptr<HasherImpl> hasher_ = std::make_shared<HasherImpl>();
  std::shared_ptr<ModuleFactoryMock> module_factory_ =
      std::make_shared<ModuleFactoryMock>();
  std::shared_ptr<runtime::RuntimeContextFactoryMock> ctx_factory;
  std::shared_ptr<boost::asio::io_context> io_ =
      std::make_shared<boost::asio::io_context>();
};

TEST_F(PvfTest, InstancesCached) {
  auto module1 = mockModule(1);
  auto module2 = mockModule(2);

  // validate with empty cache, instance with code1 for parachain 0 is cached
  module1(0);

  // instance with code1 for parachain 0 is taken from the cache
  module1(0);

  // instance with code2 for parachain 0 is cached, replacing instance for code1
  module2(0);

  // instance with code1 for parachain 1 is cached, limit of 2 instances is
  // reached
  module1(1);

  // instance with code1 for parachain 1 is taken from cache
  module1(1);

  // instance with code2 for parachain 0 is taken from cache
  module2(0);

  // instance with code1 for parachain 2 is cached, replacing instance of the
  // least recently used parachain 1
  module1(2);

  module1(0);
}
