/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/di.hpp>
#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "common/main_thread_pool.hpp"
#include "common/worker_thread_pool.hpp"
#include "mock/core/parachain/prospective_parachains_mock.hpp"
#include "mock/core/parachain/bitfield_signer_mock.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/network/peer_view_mock.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/parachain/pvf_precheck_mock.hpp"
#include "mock/core/parachain/statement_distribution_mock.hpp"
#include "mock/core/parachain/pvf_mock.hpp"
#include "mock/core/parachain/bitfield_store_mock.hpp"
#include "mock/core/parachain/availability_store_mock.hpp"
#include "mock/core/parachain/signer_factory_mock.hpp"
#include "mock/core/parachain/backing_store_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/authority_discovery/query_mock.hpp"
#include "primitives/event_types.hpp"
#include "injector/bind_by_lambda.hpp"

using namespace kagome::parachain;
namespace runtime = kagome::runtime;

using kagome::Watchdog;
using kagome::application::AppConfigurationMock;
using kagome::application::StartApp;
using kagome::common::MainThreadPool;
using kagome::common::WorkerThreadPool;
using kagome::network::PeerViewMock;
using kagome::network::PeerManagerMock;
using kagome::network::RouterMock;
using kagome::crypto::Sr25519ProviderMock;
using kagome::runtime::ParachainHostMock;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::SyncStateSubscriptionEnginePtr;
using kagome::primitives::events::SyncStateSubscriptionEngine;
using kagome::authority_discovery::QueryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::babe::BabeConfigRepositoryMock;

class BackingTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
    parachain_api_ = std::make_shared<runtime::ParachainHostMock>();

    watchdog_ = std::make_shared<Watchdog>(std::chrono::milliseconds(1));
    main_thread_pool_ = std::make_shared<MainThreadPool>(watchdog_, std::make_shared<boost::asio::io_context>());
    worker_thread_pool_ = std::make_shared<WorkerThreadPool>(watchdog_, 1);
    peer_manager_ = std::make_shared<PeerManagerMock>();
    sr25519_provider_ = std::make_shared<Sr25519ProviderMock>();
    router_ = std::make_shared<RouterMock>();
    peer_view_ = std::make_shared<PeerViewMock>();
    bitfield_signer_ = std::make_shared<BitfieldSignerMock>();
    pvf_precheck_ = std::make_shared<PvfPrecheckMock>();
    bitfield_store_ = std::make_shared<BitfieldStoreMock>();
    backing_store_ = std::make_shared<BackingStoreMock>();
    pvf_ = std::make_shared<PvfMock>();
    av_store_ = std::make_shared<AvailabilityStoreMock>();
    parachain_host_ = std::make_shared<ParachainHostMock>();
    signer_factory_ = std::make_shared<ValidatorSignerFactoryMock>();
    chain_sub_engine_ = std::make_shared<ChainSubscriptionEngine>();
    sync_state_observable_ = std::make_shared<SyncStateSubscriptionEngine>();
    query_audi_ = std::make_shared<QueryMock>();
    prospective_parachains_ = std::make_shared<ProspectiveParachainsMock>();
    block_tree_ = std::make_shared<BlockTreeMock>();
    slots_util_ = std::make_shared<SlotsUtilMock>();
    babe_config_repo_ = std::make_shared<BabeConfigRepositoryMock>();
    statement_distribution_ = std::make_shared<statement_distribution::StatementDistributionMock>();

    const auto injector = boost::di::make_injector(
               kagome::injector::bind_by_lambda<Watchdog>(
                [](const auto &injector) {
                  return std::make_shared<Watchdog>(std::chrono::milliseconds(1));
                }),
               kagome::injector::bind_by_lambda<MainThreadPool>([](const auto &injector) { return std::make_shared<MainThreadPool>(watchdog_, std::make_shared<boost::asio::io_context>()); }),
    kagome::injector::bind_by_lambda<WorkerThreadPool>([](const auto &injector) { return  std::make_shared<WorkerThreadPool>(watchdog_, 1); }),

    kagome::injector::bind_by_lambda<PeerManager>([](const auto &injector) { return  std::make_shared<PeerManagerMock>(); }),
    kagome::injector::bind_by_lambda<Sr25519Provider>([](const auto &injector) { return  std::make_shared<Sr25519ProviderMock>(); }),
    kagome::injector::bind_by_lambda<Router>([](const auto &injector) { return  std::make_shared<RouterMock>(); }),
    kagome::injector::bind_by_lambda<IPeerView>([](const auto &injector) { return  std::make_shared<PeerViewMock>(); }),
    kagome::injector::bind_by_lambda<IBitfieldSigner>([](const auto &injector) { return  std::make_shared<BitfieldSignerMock>(); }),
    kagome::injector::bind_by_lambda<IPvfPrecheck>([](const auto &injector) { return  std::make_shared<PvfPrecheckMock>(); }),
    kagome::injector::bind_by_lambda<BitfieldStore>([](const auto &injector) { return  std::make_shared<BitfieldStoreMock>(); }),
    kagome::injector::bind_by_lambda<BackingStore>([](const auto &injector) { return  std::make_shared<BackingStoreMock>(); }),
    kagome::injector::bind_by_lambda<Pvf>([](const auto &injector) { return  std::make_shared<PvfMock>(); }),
    kagome::injector::bind_by_lambda<AvailabilityStore>([](const auto &injector) { return  std::make_shared<AvailabilityStoreMock>(); }),
    kagome::injector::bind_by_lambda<ParachainHost>([](const auto &injector) { return  std::make_shared<ParachainHostMock>(); }),
    kagome::injector::bind_by_lambda<IValidatorSignerFactory>([](const auto &injector) { return  std::make_shared<ValidatorSignerFactoryMock>(); }),
    kagome::injector::bind_by_lambda<Query>([](const auto &injector) { return  std::make_shared<QueryMock>(); }),
    kagome::injector::bind_by_lambda<IProspectiveParachains>([](const auto &injector) { return  std::make_shared<ProspectiveParachainsMock>(); }),
    kagome::injector::bind_by_lambda<BlockTree>([](const auto &injector) { return  std::make_shared<BlockTreeMock>(); }),
    kagome::injector::bind_by_lambda<SlotsUtil>([](const auto &injector) { return  std::make_shared<SlotsUtilMock>(); }),
    kagome::injector::bind_by_lambda<BabeConfigRepository>([](const auto &injector) { return  std::make_shared<BabeConfigRepositoryMock>(); }),
    kagome::injector::bind_by_lambda<statement_distribution::IStatementDistribution>([](const auto &injector) { return  std::make_shared<statement_distribution::StatementDistributionMock>(); }),
    //kagome::injector::bind_by_lambda<ChainSubscriptionEngine>([](const auto &injector) { return  std::make_shared<ChainSubscriptionEngine>(); }),
    //kagome::injector::bind_by_lambda<SyncStateSubscriptionEngine>([](const auto &injector) { return  std::make_shared<SyncStateSubscriptionEngine>(); }),

    di::bind<network::ValidationObserver>.template to<parachain::ParachainObserverImpl>(),

      //di::bind<int>.to(42)[boost::di::override],
      //di::bind<double>.to(87.0),
      //useBind<Derived>()
    );


    StartApp app_state_manager;
    /*parachain_processor_ = std::make_shared<ParachainProcessorImpl>(
        peer_manager_,
        sr25519_provider_,
        router_,
        *main_thread_pool_,
        hasher_,
        peer_view_,
        *worker_thread_pool_,
        bitfield_signer_,
        pvf_precheck_,
        bitfield_store_,
        backing_store_,
        pvf_,
        av_store_,
        parachain_host_,
        signer_factory_,
        app_config_,
        app_state_manager,
        chain_sub_engine_,
        sync_state_observable_,
        query_audi_,
        prospective_parachains_,
        block_tree_,
        slots_util_,
        babe_config_repo_,
        statement_distribution_);*/
  }

  void TearDown() override {
    watchdog_->stop();
    parachain_api_.reset();
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
  std::shared_ptr<runtime::ParachainHostMock> parachain_api_;
  AppConfigurationMock app_config_;
  std::shared_ptr<Watchdog> watchdog_;
  std::shared_ptr<MainThreadPool> main_thread_pool_;
  std::shared_ptr<WorkerThreadPool> worker_thread_pool_;
  std::shared_ptr<PeerManagerMock> peer_manager_;
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider_;
  std::shared_ptr<RouterMock> router_;
  std::shared_ptr<network::PeerViewMock> peer_view_;
  std::shared_ptr<BitfieldSignerMock> bitfield_signer_;
  std::shared_ptr<PvfPrecheckMock> pvf_precheck_;
  std::shared_ptr<BitfieldStoreMock> bitfield_store_;
  std::shared_ptr<BackingStoreMock> backing_store_;
  std::shared_ptr<PvfMock> pvf_;
  std::shared_ptr<AvailabilityStoreMock> av_store_;
  std::shared_ptr<ParachainHostMock> parachain_host_;
  std::shared_ptr<ValidatorSignerFactoryMock> signer_factory_;
  std::shared_ptr<ChainSubscriptionEngine> chain_sub_engine_;
  SyncStateSubscriptionEnginePtr sync_state_observable_;
  std::shared_ptr<QueryMock> query_audi_;
  std::shared_ptr<ProspectiveParachainsMock> prospective_parachains_;
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<SlotsUtilMock> slots_util_;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_;
  std::shared_ptr<statement_distribution::StatementDistributionMock> statement_distribution_;
  std::shared_ptr<ParachainProcessorImpl> parachain_processor_;

  struct TestState {
    std::vector<ParachainId> chain_ids;

    TestState() {
      const ParachainId chain_a(1);
      const ParachainId chain_b(2);

      chain_ids = {chain_a, chain_b};
    }
  };

  struct TestLeaf {
    BlockNumber number;
    Hash hash;
    std::vector<std::pair<ParachainId, uint32_t>> min_relay_parents;
  };

  void activate_leaf(const TestLeaf &leaf, const TestState &test_state) {
    const auto &[leaf_number, leaf_hash, min_relay_parents] = leaf;
    network::ExView update{
        .view = {},
        .new_head =
            BlockHeader{
                .number = leaf_number,
                .parent_hash = get_parent_hash(leaf_hash),
                .state_root = {},
                .extrinsics_root = {},
                .digest = {},
                .hash_opt = {},
            },
        .lost = {},
    };
    update.new_head.hash_opt = leaf_hash;

    EXPECT_CALL(*parachain_api_, staging_async_backing_params(leaf_hash))
        .WillRepeatedly(Return(outcome::success(fragment::AsyncBackingParams{
            .max_candidate_depth = 4, .allowed_ancestry_len = 3})));

    const BlockNumber min_min = [&]() -> BlockNumber {
      std::optional<BlockNumber> min_min;
      for (const auto &[_, block_num] : min_relay_parents) {
        min_min = (min_min ? std::min(*min_min, block_num) : block_num);
      }
      if (min_min) {
        return *min_min;
      }
      return leaf_number;
    }();

    const auto ancestry_len = leaf_number + 1 - min_min;
    std::vector<Hash> ancestry_hashes;
    std::vector<BlockNumber> ancestry_numbers;

    Hash d = leaf_hash;
    for (BlockNumber x = 0; x < ancestry_len; ++x) {
      ancestry_hashes.emplace_back(d);
      ancestry_numbers.push_back(leaf_number - x);
      d = get_parent_hash(d);
    }
    ASSERT_EQ(ancestry_hashes.size(), ancestry_numbers.size());

    // std::cout << "---------------------------" << std::endl;
    // for(size_t i = 0; i < ancestry_hashes.size(); ++i) {
    //     const auto &h = ancestry_hashes[i];
    //     const auto &n = ancestry_numbers[i];
    //
    //    std::cout <<
    //        fmt::format("{}  {}", n, h) << std::endl;
    //}

    size_t requested_len = 0;
    for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
      const auto &hash = ancestry_hashes[i];
      const auto &number = ancestry_numbers[i];
      const auto parent_hash =
          ((i == ancestry_hashes.size() - 1) ? get_parent_hash(hash)
                                             : ancestry_hashes[i + 1]);

      EXPECT_CALL(*block_tree_, getBlockHeader(hash))
          .WillRepeatedly(Return(BlockHeader{
              .number = number,
              .parent_hash = parent_hash,
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          }));

      if (requested_len == 0) {
        //assert_matches !(
        //    virtual_overseer.recv().await,
        //    AllMessages::ProspectiveParachains(
        //        ProspectiveParachainsMessage::GetMinimumRelayParents(
        //            parent, tx)) if parent
        //        == leaf_hash =
        //        > { tx.send(min_relay_parents.clone()).unwrap(); });
      }

      requested_len += 1;
    }
  }
};

TEST_F(BackingTest, seconding_sanity_check_allowed_on_all) {
  TestState test_state;

  const BlockNumber LEAF_A_BLOCK_NUMBER = 100;
  const BlockNumber LEAF_A_ANCESTRY_LEN = 3;
  const auto para_id = test_state.chain_ids[0];

  // `a` is grandparent of `b`.
  const auto leaf_a_hash = fromNumber(130);
  const auto leaf_a_parent = get_parent_hash(leaf_a_hash);
  // const auto activated = new_leaf(leaf_a_hash, LEAF_A_BLOCK_NUMBER);
  const TestLeaf test_leaf_a{
      .number = LEAF_A_BLOCK_NUMBER,
      .hash = leaf_a_hash,
      .min_relay_parents = {{para_id,
                             LEAF_A_BLOCK_NUMBER - LEAF_A_ANCESTRY_LEN}},
  };

  const BlockNumber LEAF_B_BLOCK_NUMBER = LEAF_A_BLOCK_NUMBER + 2;
  const BlockNumber LEAF_B_ANCESTRY_LEN = 4;

  const auto leaf_b_hash = fromNumber(128);
  // let activated = new_leaf(, );
  const TestLeaf test_leaf_b{
      .number = LEAF_B_BLOCK_NUMBER,
      .hash = leaf_b_hash,
      .min_relay_parents = {{para_id,
                             LEAF_B_BLOCK_NUMBER - LEAF_B_ANCESTRY_LEN}},
  };

  activate_leaf(test_leaf_a, test_state);
  activate_leaf(test_leaf_b, test_state);
}
