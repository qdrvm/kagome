/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/main_thread_pool.hpp"
#include "common/worker_thread_pool.hpp"
#include "core/parachain/parachain_test_harness.hpp"
#include "utils/map.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "injector/bind_by_lambda.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/authority_discovery/query_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/core/network/peer_view_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "mock/core/parachain/availability_store_mock.hpp"
#include "mock/core/parachain/backing_store_mock.hpp"
#include "mock/core/parachain/bitfield_signer_mock.hpp"
#include "mock/core/parachain/bitfield_store_mock.hpp"
#include "mock/core/parachain/prospective_parachains_mock.hpp"
#include "mock/core/parachain/pvf_mock.hpp"
#include "mock/core/parachain/pvf_precheck_mock.hpp"
#include "mock/core/parachain/signer_factory_mock.hpp"
#include "mock/core/parachain/statement_distribution_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "primitives/event_types.hpp"
#include "testutil/lazy.hpp"

using namespace kagome::parachain;
namespace runtime = kagome::runtime;

using kagome::Watchdog;
using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationMock;
using kagome::application::StartApp;
using kagome::authority_discovery::Query;
using kagome::authority_discovery::QueryMock;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::common::MainThreadPool;
using kagome::common::WorkerThreadPool;
using kagome::consensus::SlotsUtil;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::babe::BabeConfigRepository;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::crypto::Sr25519Provider;
using kagome::crypto::Sr25519ProviderMock;
using kagome::network::IPeerView;
using kagome::network::PeerManager;
using kagome::network::PeerManagerMock;
using kagome::network::PeerViewMock;
using kagome::network::Router;
using kagome::network::RouterMock;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::SyncStateSubscriptionEngine;
using kagome::primitives::events::SyncStateSubscriptionEnginePtr;
using kagome::runtime::ParachainHost;
using kagome::runtime::ParachainHostMock;

class BackingTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();

    watchdog_ = std::make_shared<Watchdog>(std::chrono::milliseconds(1));
    main_thread_pool_ = std::make_shared<MainThreadPool>(
        watchdog_, std::make_shared<boost::asio::io_context>());
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
    statement_distribution_ =
        std::make_shared<statement_distribution::StatementDistributionMock>();
    signer_ = std::make_shared<ValidatorSignerMock>();

    my_view_observable_ =
        std::make_shared<PeerViewMock::MyViewSubscriptionEngine>();

    StartApp app_state_manager;
    parachain_processor_ = std::make_shared<ParachainProcessorImpl>(
        peer_manager_,
        sr25519_provider_,
        router_,
        hasher_,
        peer_view_,
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
        testutil::sptr_to_lazy<SlotsUtil>(slots_util_),
        babe_config_repo_,
        statement_distribution_);

    EXPECT_CALL(*peer_view_, getMyViewObservable())
        .WillRepeatedly(Return(my_view_observable_));
    EXPECT_CALL(*statement_distribution_, store_parachain_processor(testing::_))
        .Times(1);
    EXPECT_CALL(*bitfield_signer_, setBroadcastCallback(testing::_)).Times(1);
    EXPECT_CALL(app_config_, roles())
        .WillRepeatedly(Return(network::Roles(0xff)));
    EXPECT_CALL(*prospective_parachains_, getBlockTree())
        .WillRepeatedly(Return(block_tree_));

    app_state_manager.start();
  }

  void TearDown() override {
    watchdog_->stop();
    parachain_host_.reset();
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
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
  std::shared_ptr<statement_distribution::StatementDistributionMock>
      statement_distribution_;
  std::shared_ptr<ParachainProcessorImpl> parachain_processor_;
  std::shared_ptr<ValidatorSignerMock> signer_;

  PeerViewMock::MyViewSubscriptionEnginePtr my_view_observable_;

  struct TestState {
    std::vector<ParachainId> chain_ids;
    std::unordered_map<ParachainId, HeadData> head_data;
    std::vector<crypto::Sr25519PublicKey> validators;
    std::vector<runtime::CoreState> availability_cores;
    SigningContext signing_context;
    uint32_t minimum_backing_votes;

    struct {
      std::vector<runtime::ValidatorGroup> groups;
      runtime::GroupDescriptor group_rotation;
    } validator_groups;

    TestState() {
      const ParachainId chain_a(1);
      const ParachainId chain_b(2);

      chain_ids = {chain_a, chain_b};

      head_data[chain_a] = {4, 5, 6};
      head_data[chain_b] = {5, 6, 7};

      crypto::Bip39ProviderImpl bip_provider{
          std::make_shared<crypto::Pbkdf2ProviderImpl>(),
          std::make_shared<crypto::HasherImpl>(),
      };

      auto sr25519_provider = std::make_shared<crypto::Sr25519ProviderImpl>();
      auto f = [&](std::string_view phrase) {
        auto bip = bip_provider.generateSeed(phrase).value();
        auto keys = sr25519_provider
                        ->generateKeypair(crypto::Sr25519Seed::from(bip.seed),
                                          bip.junctions)
                        .value();
        return keys.public_key;
      };
      validators.emplace_back(f("//Alice"));
      validators.emplace_back(f("//Bob"));
      validators.emplace_back(f("//Charlie"));
      validators.emplace_back(f("//Dave"));
      validators.emplace_back(f("//Ferdie"));
      validators.emplace_back(f("//One"));

      validator_groups.groups = {runtime::ValidatorGroup{
                                     .validators = {2, 0, 3, 5},
                                 },
                                 runtime::ValidatorGroup{
                                     .validators = {1},
                                 }};
      validator_groups.group_rotation = runtime::GroupDescriptor{
          .session_start_block = 0,
          .group_rotation_frequency = 100,
          .now_block_num = 1,
      };

      availability_cores = {runtime::ScheduledCore{
                                .para_id = chain_a,
                                .collator = std::nullopt,
                            },
                            runtime::ScheduledCore{
                                .para_id = chain_b,
                                .collator = std::nullopt,
                            }};

      const auto relay_parent = fromNumber(5);

      signing_context = SigningContext{
          .session_index = 1,
          .relay_parent = relay_parent,
      };

      minimum_backing_votes = LEGACY_MIN_BACKING_VOTES;
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

    EXPECT_CALL(*parachain_host_, staging_async_backing_params(leaf_hash))
        .WillRepeatedly(Return(outcome::success(fragment::AsyncBackingParams{
            .max_candidate_depth = 4, .allowed_ancestry_len = 3})));

    EXPECT_CALL(*prospective_parachains_, prospectiveParachainsMode(leaf_hash))
        .WillRepeatedly(Return(ProspectiveParachainsMode{
            .max_candidate_depth = 4,
            .allowed_ancestry_len = 3,
        }));

    EXPECT_CALL(*signer_, validatorIndex()).WillRepeatedly(Return(0));

    EXPECT_CALL(*bitfield_store_, printStoragesLoad()).WillRepeatedly(Return());
    EXPECT_CALL(*backing_store_, printStoragesLoad()).WillRepeatedly(Return());
    EXPECT_CALL(*av_store_, printStoragesLoad()).WillRepeatedly(Return());
    EXPECT_CALL(*prospective_parachains_, printStoragesLoad())
        .WillRepeatedly(Return());

    EXPECT_CALL(*backing_store_, onActivateLeaf(testing::_))
        .WillRepeatedly(Return());
    EXPECT_CALL(*prospective_parachains_, onActiveLeavesUpdate(testing::_))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*peer_manager_, enumeratePeerState(testing::_))
        .WillRepeatedly(Return());

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

    size_t requested_len = 0;
    for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
      const auto &hash = ancestry_hashes[i];
      const auto &number = ancestry_numbers[i];
      const auto parent_hash =
          ((i == ancestry_hashes.size() - 1) ? get_parent_hash(hash)
                                             : ancestry_hashes[i + 1]);

      EXPECT_CALL(*parachain_host_, session_index_for_child(hash))
          .WillRepeatedly(Return(test_state.signing_context.session_index));

      EXPECT_CALL(*block_tree_, getBlockHeader(hash))
          .WillRepeatedly(Return(BlockHeader{
              .number = number,
              .parent_hash = parent_hash,
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          }));

      EXPECT_CALL(*parachain_host_, validators(hash))
          .WillRepeatedly(Return(test_state.validators));

      EXPECT_CALL(*parachain_host_, validator_groups(hash))
          .WillRepeatedly(Return(
              std::make_tuple(test_state.validator_groups.groups,
                              runtime::GroupDescriptor{
                                  .session_start_block =
                                      test_state.validator_groups.group_rotation
                                          .session_start_block,
                                  .group_rotation_frequency =
                                      test_state.validator_groups.group_rotation
                                          .group_rotation_frequency,
                                  .now_block_num = number,
                              })));

      EXPECT_CALL(*parachain_host_, availability_cores(hash))
          .WillRepeatedly(Return(test_state.availability_cores));

      EXPECT_CALL(*signer_factory_, at(hash)).WillRepeatedly(Return(signer_));
      EXPECT_CALL(*signer_factory_, getAuthorityValidatorIndex(hash))
          .WillRepeatedly(Return(0));

      runtime::SessionInfo si;
      si.validators = test_state.validators;
      si.discovery_keys = test_state.validators;
      EXPECT_CALL(*parachain_host_,
                  session_info(hash, test_state.signing_context.session_index))
          .WillRepeatedly(Return(si));

      EXPECT_CALL(*parachain_host_,
                  node_features(hash))
          .WillRepeatedly(Return(runtime::NodeFeatures()));

      EXPECT_CALL(
          *parachain_host_,
          minimum_backing_votes(hash, test_state.signing_context.session_index))
          .WillRepeatedly(Return(test_state.minimum_backing_votes));

      if (requested_len == 0) {
        EXPECT_CALL(*prospective_parachains_,
                    answerMinimumRelayParentsRequest(leaf_hash))
            .WillRepeatedly(Return(min_relay_parents));
      }

      requested_len += 1;
    }

    my_view_observable_->notify(PeerViewMock::EventType::kViewUpdated, update);
  }

  runtime::PersistedValidationData dummy_pvd() {
    return runtime::PersistedValidationData{
        .parent_head = {7, 8, 9},
        .relay_parent_number = 0,
        .relay_parent_storage_root = fromNumber(0),
        .max_pov_size = 1024,
    };
  }

  template <typename T>
  inline Hash hash_of(T &&t) {
    return hasher_->blake2b_256(scale::encode(std::forward<T>(t)).value());
  }

  template <typename T>
  static Hash hash_of(const kagome::crypto::Hasher &hasher, T &&t) {
    return hasher.blake2b_256(scale::encode(std::forward<T>(t)).value());
  }

  Hash make_erasure_root(
      const TestState &test,
      const network::PoV &pov,
      const runtime::PersistedValidationData &validation_data) {
    const runtime::AvailableData available_data{
        .pov = pov,
        .validation_data = validation_data,
    };

    auto chunks = toChunks(test.validators.size(), available_data).value();
    return makeTrieProof(chunks);
  }

  void assert_validation_requests(
      const runtime::ValidationCode &validation_code) {}


void assert_hypothetical_membership_requests(std::vector<std::pair<IProspectiveParachains::HypotheticalMembershipRequest, std::vector<std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>>> expected_requests) {
    for (const auto &[request, candidates_membership] : expected_requests) {
        EXPECT_CALL(*prospective_parachains_, answer_hypothetical_membership_request(request)) //request.candidates, ref
            .WillOnce(Return(candidates_membership));

    }
}

std::vector<std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>> make_hypothetical_membership_response(
	const HypotheticalCandidate &hypothetical_candidate,
	const Hash &relay_parent_hash
) {
	return {{hypothetical_candidate, {relay_parent_hash}}};
}


  void assert_validate_seconded_candidate(
      const Hash &relay_parent,
      const network::CommittedCandidateReceipt &candidate,
      const network::PoV &assert_pov,
      const runtime::PersistedValidationData &assert_pvd,
      const runtime::ValidationCode &assert_validation_code,
      const HeadData &expected_head_data,
      bool fetch_pov) {
    assert_validation_requests(assert_validation_code);

    const std::pair<network::CandidateCommitments, runtime::PersistedValidationData> pvf_result{
				network::CandidateCommitments {
					.upward_msgs = {},
					.outbound_hor_msgs = {},
					.opt_para_runtime = std::nullopt,
					.para_head = expected_head_data,
					.downward_msgs_count = 0,
					.watermark = 0,
				},
				assert_pvd
    };
    EXPECT_CALL(*pvf_, call_pvf(candidate.to_plain(*hasher_), assert_pov, assert_pvd))
        .WillRepeatedly(Return(pvf_result));

    EXPECT_CALL(*av_store_, storeData(testing::_, network::candidateHash(*hasher_, candidate), testing::_, assert_pov, assert_pvd))
        .WillRepeatedly(Return());
    
  }

  struct TestCandidateBuilder {
    ParachainId para_id;
    HeadData head_data;
    Hash pov_hash;
    Hash relay_parent;
    Hash erasure_root;
    Hash persisted_validation_data_hash;
    std::vector<uint8_t> validation_code;

    network::CommittedCandidateReceipt build(
        const kagome::crypto::Hasher &hasher) {
      return network::CommittedCandidateReceipt{
          .descriptor =
              network::CandidateDescriptor{
                  .para_id = para_id,
                  .relay_parent = relay_parent,
                  .reserved_1 = {},
                  .persisted_data_hash = persisted_validation_data_hash,
                  .pov_hash = pov_hash,
                  .erasure_encoding_root = erasure_root,
                  .reserved_2 = {},
                  .para_head_hash = hash_of(hasher, head_data),
                  .validation_code_hash = hash_of(
                      hasher, kagome::runtime::ValidationCode(validation_code)),
              },
          .commitments =
              network::CandidateCommitments{
                  .upward_msgs = {},
                  .outbound_hor_msgs = {},
                  .opt_para_runtime = std::nullopt,
                  .para_head = head_data,
                  .downward_msgs_count = 0,
                  .watermark = 0,
              },
      };
    }
  };
};

TEST_F(BackingTest, seconding_sanity_check_allowed_on_all) {
  TestState test_state;

  const BlockNumber LEAF_A_BLOCK_NUMBER = 100;
  const BlockNumber LEAF_A_ANCESTRY_LEN = 3;
  const auto para_id = test_state.chain_ids[0];

  // `a` is grandparent of `b`.
  const auto leaf_a_hash = fromNumber(130);
  const auto leaf_a_parent = get_parent_hash(leaf_a_hash);
  const TestLeaf test_leaf_a{
      .number = LEAF_A_BLOCK_NUMBER,
      .hash = leaf_a_hash,
      .min_relay_parents = {{para_id,
                             LEAF_A_BLOCK_NUMBER - LEAF_A_ANCESTRY_LEN}},
  };

  const BlockNumber LEAF_B_BLOCK_NUMBER = LEAF_A_BLOCK_NUMBER + 2;
  const BlockNumber LEAF_B_ANCESTRY_LEN = 4;

  const auto leaf_b_hash = fromNumber(128);
  const TestLeaf test_leaf_b{
      .number = LEAF_B_BLOCK_NUMBER,
      .hash = leaf_b_hash,
      .min_relay_parents = {{para_id,
                             LEAF_B_BLOCK_NUMBER - LEAF_B_ANCESTRY_LEN}},
  };

  sync_state_observable_->notify(
      kagome::primitives::events::SyncStateEventType::kSyncState,
      kagome::primitives::events::SyncStateEventParams::SYNCHRONIZED);

  activate_leaf(test_leaf_a, test_state);
  activate_leaf(test_leaf_b, test_state);

  kagome::network::PoV pov{.payload = {42, 43, 44}};
  const auto pvd = dummy_pvd();
  kagome::runtime::ValidationCode validation_code = {1, 2, 3};

  const auto &expected_head_data = test_state.head_data[para_id];
  const auto pov_hash = hash_of(pov);

  const auto candidate =
      TestCandidateBuilder{
          .para_id = para_id,
          .head_data = expected_head_data,
          .pov_hash = pov_hash,
          .relay_parent = leaf_a_parent,
          .erasure_root = make_erasure_root(test_state, pov, pvd),
          .persisted_validation_data_hash = hash_of(pvd),
          .validation_code = validation_code,
      }
          .build(*hasher_);

  assert_validate_seconded_candidate(leaf_a_parent,
                                     candidate,
                                     pov,
                                     pvd,
                                     validation_code,
                                     expected_head_data,
                                     false);

		// `seconding_sanity_check`
		const HypotheticalCandidateComplete hypothetical_candidate{
			.candidate_hash =  network::candidateHash(*hasher_, candidate),
			.receipt =  candidate,
			.persisted_validation_data = pvd,
		};
		const IProspectiveParachains::HypotheticalMembershipRequest expected_request_a {
			.candidates = {hypothetical_candidate},
			.fragment_chain_relay_parent = leaf_a_hash,
		};
		const auto expected_response_a =
			make_hypothetical_membership_response(hypothetical_candidate, leaf_a_hash);

		const IProspectiveParachains::HypotheticalMembershipRequest expected_request_b {
			.candidates = {hypothetical_candidate},
			.fragment_chain_relay_parent = leaf_b_hash,
		};
		const auto expected_response_b =
			make_hypothetical_membership_response(hypothetical_candidate, leaf_b_hash);

		assert_hypothetical_membership_requests(
			{
				{expected_request_a, expected_response_a},
				{expected_request_b, expected_response_b}
            }
		);

        const network::CommittedCandidateReceipt receipt{
                  .descriptor = candidate.descriptor,
                  .commitments = network::CandidateCommitments {
					.upward_msgs = {},
					.outbound_hor_msgs = {},
					.opt_para_runtime = std::nullopt,
					.para_head = expected_head_data,
					.downward_msgs_count = 0,
					.watermark = 0,
				},
                };
        network::Statement statement {
              network::CandidateState{receipt}};

        IndexedAndSigned<network::Statement> signed_statement {
            .payload = {
                .payload = statement,
                .ix = 0,
            },
            .signature = {},
        };

        EXPECT_CALL(*signer_, sign(statement))
            .WillOnce(Return(signed_statement));

        const auto candidate_hash = network::candidateHash(*hasher_, candidate);        
        EXPECT_CALL(*prospective_parachains_, introduce_seconded_candidate(para_id, candidate, testing::_, candidate_hash)) //request.candidates, ref
            .WillOnce(Return(true));

        EXPECT_CALL(*statement_distribution_, share_local_statement(leaf_a_parent, testing::_))
            .WillOnce(Return());

        BackingStore::ImportResult import_result {
            .candidate = candidate_hash,
            .group_id = 0,
            .validity_votes = 1,
        };

        EXPECT_CALL(*backing_store_, put(leaf_a_parent, testing::_, testing::_, signed_statement, testing::_))
            .WillOnce(Return(import_result));

        BackingStore::StatementInfo stmt_info {
            .group_id = 0,
            .candidate = receipt,
            .validity_votes = {{0, BackingStore::ValidityVoteIssued{}}},
        };
        EXPECT_CALL(*backing_store_, getCadidateInfo(leaf_a_parent, candidate_hash))
            .WillOnce(Return(std::cref(stmt_info)));

  parachain_processor_->handle_second_message(
      candidate.to_plain(*hasher_), pov, pvd, leaf_a_hash);
}
