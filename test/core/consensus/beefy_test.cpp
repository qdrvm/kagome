/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/asio/bind_executor.hpp>
#include <libp2p/basic/scheduler.hpp>

#include "common/main_thread_pool.hpp"
#include "consensus/beefy/digest.hpp"
#include "consensus/beefy/impl/beefy_impl.hpp"
#include "consensus/beefy/impl/beefy_thread_pool.hpp"
#include "consensus/beefy/sig.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/application/chain_spec_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/beefy/fetch_justification.hpp"
#include "mock/core/consensus/timeline/timeline_mock.hpp"
#include "mock/core/crypto/session_keys_mock.hpp"
#include "mock/core/network/protocols/beefy_protocol_mock.hpp"
#include "mock/core/network/synchronizer_mock.hpp"
#include "mock/core/offchain/offchain_worker_factory_mock.hpp"
#include "mock/core/offchain/offchain_worker_pool_mock.hpp"
#include "mock/core/runtime/beefy_api.hpp"
#include "network/impl/protocols/beefy_protocol_impl.hpp"
#include "primitives/event_types.hpp"
#include "storage/in_memory/in_memory_spaced_storage.hpp"
#include "testutil/lazy.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::beefyMmrDigest;
using kagome::TestThreadPool;
using kagome::application::ChainSpecMock;
using kagome::application::StartApp;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::common::MainThreadPool;
using kagome::consensus::Timeline;
using kagome::consensus::TimelineMock;
using kagome::consensus::beefy::AuthoritySetId;
using kagome::consensus::beefy::BeefyGossipMessage;
using kagome::consensus::beefy::BeefyJustification;
using kagome::consensus::beefy::Commitment;
using kagome::consensus::beefy::ConsensusDigest;
using kagome::consensus::beefy::FetchJustification;
using kagome::consensus::beefy::FetchJustificationMock;
using kagome::consensus::beefy::kMmr;
using kagome::consensus::beefy::MmrRootHash;
using kagome::consensus::beefy::SignedCommitment;
using kagome::consensus::beefy::ValidatorSet;
using kagome::consensus::beefy::VoteMessage;
using kagome::crypto::EcdsaKeypair;
using kagome::crypto::EcdsaProviderImpl;
using kagome::crypto::EcdsaSeed;
using kagome::crypto::HasherImpl;
using kagome::crypto::SecureBuffer;
using kagome::crypto::SessionKeysMock;
using kagome::network::BeefyImpl;
using kagome::network::BeefyProtocol;
using kagome::network::BeefyProtocolMock;
using kagome::network::BeefyThreadPool;
using kagome::network::SynchronizerMock;
using kagome::offchain::OffchainWorkerFactoryMock;
using kagome::offchain::OffchainWorkerPoolMock;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::primitives::Consensus;
using kagome::primitives::kBeefyEngineId;
using kagome::primitives::OpaqueKeyOwnershipProof;
using kagome::primitives::events::ChainEventType;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::ChainSubscriptionEnginePtr;
using kagome::runtime::BeefyApiMock;
using kagome::scale::encode;
using kagome::storage::InMemorySpacedStorage;
using testing::_;
using testing::Return;

struct Timer : libp2p::basic::Scheduler {
  std::chrono::milliseconds now() const override {
    abort();
  }
  Handle scheduleImpl(Callback &&cb, std::chrono::milliseconds, bool) override {
    cb_.emplace(std::move(cb));
    return Handle{};
  }

  void call() {
    if (cb_) {
      auto cb = std::move(*cb_);
      cb_.reset();
      cb();
    }
  }

  std::optional<Callback> cb_;
};

struct BeefyPeer {
  bool vote_ = true;
  bool listen_ = true;
  std::shared_ptr<EcdsaKeypair> keys_;
  BlockNumber finalized_ = 0;
  std::vector<BlockNumber> justifications_;

  std::shared_ptr<InMemorySpacedStorage> db_ =
      std::make_shared<InMemorySpacedStorage>();
  std::shared_ptr<BlockTreeMock> block_tree_ =
      std::make_shared<BlockTreeMock>();
  std::shared_ptr<Timer> timer_ = std::make_shared<Timer>();
  std::shared_ptr<SessionKeysMock> keystore_ =
      std::make_shared<SessionKeysMock>();
  std::shared_ptr<BeefyProtocolMock> broadcast_ =
      std::make_shared<BeefyProtocolMock>();
  std::shared_ptr<FetchJustificationMock> fetch_ =
      std::make_shared<FetchJustificationMock>();
  ChainSubscriptionEnginePtr chain_sub_ =
      std::make_shared<ChainSubscriptionEngine>();
  std::shared_ptr<BeefyImpl> beefy_;
};

struct BeefyTest : testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(chain_spec_, beefyMinDelta()).WillRepeatedly([this] {
      return min_delta_;
    });
    EXPECT_CALL(*beefy_api_, genesis(_)).WillRepeatedly([this] {
      return genesis_;
    });
    EXPECT_CALL(*beefy_api_, validatorSet(_)).WillRepeatedly([this] {
      return genesisVoters();
    });
    EXPECT_CALL(*timeline_, wasSynchronized()).WillRepeatedly(Return(true));
    EXPECT_CALL(*synchronizer_, fetchHeadersBack(_, _, _, _))
        .WillRepeatedly(Return(true));
  }

  void reloadPeer(BeefyPeer &peer) {
    auto app_state_manager = std::make_shared<StartApp>();
    EXPECT_CALL(*app_state_manager, atShutdown(_)).Times(testing::AnyNumber());
    std::shared_ptr<MainThreadPool> main_thread_pool_ =
        std::make_shared<MainThreadPool>(TestThreadPool{io_});
    std::shared_ptr<BeefyThreadPool> beefy_thread_pool_ =
        std::make_shared<BeefyThreadPool>(TestThreadPool{io_});
    peer.beefy_ = std::make_shared<BeefyImpl>(
        app_state_manager,
        chain_spec_,
        peer.block_tree_,
        beefy_api_,
        ecdsa_,
        peer.db_,
        *main_thread_pool_,
        *beefy_thread_pool_,
        peer.timer_,
        testutil::sptr_to_lazy<Timeline>(timeline_),
        peer.keystore_,
        testutil::sptr_to_lazy<BeefyProtocol>(peer.broadcast_),
        testutil::sptr_to_lazy<FetchJustification>(peer.fetch_),
        offchain_worker_factory_,
        offchain_worker_pool_,
        peer.chain_sub_,
        testutil::sptr_to_lazy<kagome::network::Synchronizer>(synchronizer_));
    app_state_manager->start();
  }

  void makePeers(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) {
      auto &peer = *peers_.emplace_back(std::make_shared<BeefyPeer>());

      SecureBuffer<> seed_buf(EcdsaSeed::size());
      seed_buf.at(0) = i;
      seed_buf.at(1) = 1;
      auto seed = EcdsaSeed::from(std::move(seed_buf)).value();
      peer.keys_ = std::make_shared<EcdsaKeypair>(
          std::move(ecdsa_->generateKeypair(seed, {}).value()));
      EXPECT_CALL(*peer.keystore_, getBeefKeyPair(_))
          .WillRepeatedly([this, i]() {
            return peers_.at(i)->vote_ ? std::make_optional(std::make_pair(
                                             peers_.at(i)->keys_, i))
                                       : std::nullopt;
          });

      EXPECT_CALL(*peer.block_tree_, bestBlock()).WillRepeatedly([this] {
        return blocks_.back().blockInfo();
      });

      EXPECT_CALL(*peer.block_tree_, getLastFinalized())
          .WillRepeatedly([this, i] {
            return blocks_[peers_.at(i)->finalized_].blockInfo();
          });
      EXPECT_CALL(*peer.block_tree_, getBlockHash(_))
          .WillRepeatedly(
              [this](BlockNumber i) { return blocks_.at(i).hash(); });
      EXPECT_CALL(*peer.block_tree_, getBlockHeader(_))
          .WillRepeatedly([this](BlockHash h) {
            for (auto &block : blocks_) {
              if (block.hash() == h) {
                return block;
              }
            }
            throw std::logic_error{"getBlockHeader"};
          });
      EXPECT_CALL(*peer.broadcast_, broadcast(_))
          .WillRepeatedly([this, i](std::shared_ptr<BeefyGossipMessage> m) {
            if (auto jr = boost::get<BeefyJustification>(&*m)) {
              auto &j = boost::get<SignedCommitment>(*jr);
              justifications_.emplace(j.commitment.block_number, *jr);
              peers_.at(i)->justifications_.emplace_back(
                  j.commitment.block_number);
            }
            post(*io_, [this, i, m] {
              for (auto &peer2 : peers_) {
                if (peer2 != peers_.at(i) and peer2->listen_) {
                  peer2->beefy_->onMessage(*m);
                }
              }
            });
          });
      EXPECT_CALL(*peer.fetch_, fetchJustification(_))
          .WillRepeatedly(bind_executor(*io_, [this, i](BlockNumber block) {
            auto it = justifications_.find(block);
            if (it == justifications_.end()) {
              return;
            }
            peers_.at(i)->beefy_->onMessage(it->second);
          }));
      reloadPeer(peer);
    }
  }

  ValidatorSet genesisVoters() {
    ValidatorSet voters;
    for (auto &peer : peers_) {
      voters.validators.emplace_back(peer->keys_->public_key);
    }
    return voters;
  }

  void generate_blocks_and_sync(BlockNumber max, BlockNumber session) {
    auto voters = genesisVoters();
    BlockHash parent;
    for (BlockNumber i = 0; i <= max; ++i) {
      BlockHeader block;
      block.number = i;
      if (i > 0) {
        auto &mmr = parent;
        block.digest.emplace_back(
            Consensus{kBeefyEngineId, ConsensusDigest{mmr}});
      }
      if (i > genesis_ and i % session == 0) {
        ++voters.id;
        block.digest.emplace_back(
            Consensus{kBeefyEngineId, ConsensusDigest{voters}});
      }
      calculateBlockHash(block, *hasher_);
      blocks_.emplace_back(block);
      parent = block.hash();
    }
  }

  auto all() {
    std::set<size_t> peers;
    for (size_t i = 0; i < peers_.size(); ++i) {
      peers.emplace(i);
    }
    return peers;
  }

  void finalize(std::set<size_t> peers, BlockNumber finalized) {
    for (auto &i : peers) {
      peers_.at(i)->finalized_ = finalized;
      peers_.at(i)->chain_sub_->notify(ChainEventType::kFinalizedHeads,
                                       blocks_.at(finalized));
    }
  }

  void loop() {
    io_->restart();
    io_->run();
  }

  void expect(std::set<size_t> peers, std::vector<BlockNumber> expected) {
    for (auto &i : peers) {
      EXPECT_EQ(peers_.at(i)->justifications_, expected);
      peers_.at(i)->justifications_.clear();
    }
  }

  void rebroadcast() {
    for (auto &peer : peers_) {
      peer->timer_->call();
    }
  }

  void finalize_block_and_wait_for_beefy(std::set<size_t> peers,
                                         BlockNumber finalized,
                                         std::vector<BlockNumber> expected) {
    finalize(peers, finalized);
    loop();
    expect(peers, expected);
  }

  void finalize_block_and_wait_for_beefy(BlockNumber finalized,
                                         std::vector<BlockNumber> expected) {
    finalize_block_and_wait_for_beefy(all(), finalized, expected);
  }

  VoteMessage signVote(const BeefyPeer &peer, Commitment commitment) const {
    return VoteMessage{
        .commitment = commitment,
        .id = peer.keys_->public_key,
        .signature =
            ecdsa_->signPrehashed(prehash(commitment), peer.keys_->secret_key)
                .value(),
    };
  }

  ChainSpecMock chain_spec_;
  std::shared_ptr<BeefyApiMock> beefy_api_ = std::make_shared<BeefyApiMock>();
  std::shared_ptr<HasherImpl> hasher_ = std::make_shared<HasherImpl>();
  std::shared_ptr<EcdsaProviderImpl> ecdsa_ =
      std::make_shared<EcdsaProviderImpl>(hasher_);
  std::shared_ptr<boost::asio::io_context> io_ =
      std::make_shared<boost::asio::io_context>();
  std::shared_ptr<TimelineMock> timeline_ = std::make_shared<TimelineMock>();
  std::shared_ptr<OffchainWorkerFactoryMock> offchain_worker_factory_ =
      std::make_shared<OffchainWorkerFactoryMock>();
  std::shared_ptr<OffchainWorkerPoolMock> offchain_worker_pool_ =
      std::make_shared<OffchainWorkerPoolMock>();
  std::shared_ptr<SynchronizerMock> synchronizer_ =
      std::make_shared<SynchronizerMock>();

  std::vector<BlockHeader> blocks_;
  BlockNumber min_delta_ = 1;
  BlockNumber genesis_ = 1;
  std::vector<std::shared_ptr<BeefyPeer>> peers_;
  std::map<BlockNumber, BeefyJustification> justifications_;
};

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L613
TEST_F(BeefyTest, beefy_finalizing_blocks) {
  min_delta_ = 4;
  makePeers(2);
  generate_blocks_and_sync(42, 10);

  // finalize block #5 -> BEEFY should finalize #1 (mandatory) and #5 from
  // diff-power-of-two rule.
  finalize_block_and_wait_for_beefy(1, {1});
  finalize_block_and_wait_for_beefy(5, {5});

  // GRANDPA finalize #10 -> BEEFY finalize #10 (mandatory)
  finalize_block_and_wait_for_beefy(10, {10});

  // GRANDPA finalize #18 -> BEEFY finalize #14, then #18 (diff-power-of-two
  // rule)
  finalize_block_and_wait_for_beefy(18, {14, 18});

  // GRANDPA finalize #20 -> BEEFY finalize #20 (mandatory)
  finalize_block_and_wait_for_beefy(20, {20});

  // GRANDPA finalize #21 -> BEEFY finalize nothing (yet) because min delta
  finalize_block_and_wait_for_beefy(21, {});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L653
TEST_F(BeefyTest, lagging_validators) {
  makePeers(3);
  generate_blocks_and_sync(62, 30);

  // finalize block #15 -> BEEFY should finalize #1 (mandatory) and #9, #13,
  // #14, #15 from diff-power-of-two rule.
  finalize_block_and_wait_for_beefy(1, {1});
  finalize_block_and_wait_for_beefy(15, {9, 13, 14, 15});

  // Alice and Bob finalize #25, Charlie lags behind
  finalize({0, 1}, 25);
  loop();
  // verify nothing gets finalized by BEEFY
  expect(all(), {});

  // Charlie catches up and also finalizes #25
  finalize({2}, 25);
  rebroadcast();
  loop();
  // expected beefy finalizes blocks 23, 24, 25 from diff-power-of-two
  expect(all(), {23, 24, 25});

  // Both finalize #30 (mandatory session) and #32 -> BEEFY finalize #30
  // (mandatory), #31, #32
  finalize_block_and_wait_for_beefy(30, {30});
  finalize_block_and_wait_for_beefy(32, {31, 32});

  // Verify that session-boundary votes get buffered by client and only
  // processed once session-boundary block is GRANDPA-finalized (this guarantees
  // authenticity for the new session validator set).

  // Alice and Bob finalize session-boundary mandatory block #60, Charlie lags
  // behind
  finalize({0, 1}, 60);
  // verify nothing gets finalized by BEEFY
  expect(all(), {});

  // Charlie catches up and also finalizes #60 (and should have buffered Alice's
  // vote on #60)
  finalize({2}, 60);
  rebroadcast();
  loop();
  // verify beefy skips intermediary votes, and successfully finalizes mandatory
  // block #60
  expect(all(), {60});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L721
TEST_F(BeefyTest, correct_beefy_payload) {
  min_delta_ = 2;
  makePeers(4);
  generate_blocks_and_sync(12, 20);

  // Alice, Bob, Charlie will vote on good payloads
  // Dave will vote on bad mmr roots
  peers_.at(3)->vote_ = false;

  // with 3 good voters and 1 bad one, consensus should happen and best blocks
  // produced.
  finalize_block_and_wait_for_beefy(1, {1});
  finalize_block_and_wait_for_beefy(10, {9});

  // now 2 good validators and 1 bad one are voting
  finalize({0, 1, 3}, 11);
  Commitment commitment{{}, 11, 0};
  auto vote = signVote(*peers_.at(3), commitment);
  for (auto &peer : peers_) {
    peer->beefy_->onMessage(vote);
  }
  loop();
  // verify consensus is _not_ reached
  expect(all(), {});

  // 3rd good validator catches up and votes as well
  finalize({2}, 11);
  rebroadcast();
  loop();
  // verify consensus is reached
  expect(all(), {11});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L779
TEST_F(BeefyTest, beefy_importing_justifications) {
  genesis_ = 3;
  AuthoritySetId set = 0;
  makePeers(1);
  auto &peer = peers_.at(0);
  peer->vote_ = false;
  generate_blocks_and_sync(genesis_ + 1, 10);
  finalize_block_and_wait_for_beefy(genesis_ + 1, {});
  auto justify = [&](BlockNumber block_number, AuthoritySetId set) {
    auto mmr = beefyMmrDigest(blocks_.at(block_number));
    Commitment commitment{{{kMmr, Buffer{*mmr}}}, block_number, set};
    auto vote = signVote(*peer, commitment);
    peer->beefy_->onJustification(
        blocks_.at(block_number).hash(),
        {Buffer{encode(BeefyJustification{
                           SignedCommitment{commitment, {vote.signature}}})
                    .value()}});
  };

  // Import block 2 with "valid" justification (beefy pallet genesis block not
  // yet reached).
  justify(genesis_ - 1, set);
  EXPECT_EQ(peer->beefy_->finalized(), 0);

  // Import block 3 with valid justification.
  justify(genesis_, set);
  loop();
  EXPECT_EQ(peer->beefy_->finalized(), genesis_);

  // Import block 4 with invalid justification (incorrect validator set).
  justify(genesis_ + 1, set + 1);
  loop();
  EXPECT_EQ(peer->beefy_->finalized(), genesis_);
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L944
TEST_F(BeefyTest, on_demand_beefy_justification_sync) {
  min_delta_ = 4;
  // Alice, Bob, Charlie start first and make progress through voting.
  makePeers(4);
  // Dave will start late and have to catch up using on-demand justification
  // requests (since in this test there is no block import queue to
  // automatically import justifications). Instantiate but don't run Dave, yet.
  size_t dave{3};
  peers_.at(dave)->listen_ = false;

  // push 30 blocks
  generate_blocks_and_sync(30, 5);

  // With 3 active voters and one inactive, consensus should happen and blocks
  // BEEFY-finalized. Need to finalize at least one block in each session,
  // choose randomly.
  std::set<size_t> fast_peers{0, 1, 2};
  // finalize_block_and_wait_for_beefy({0, 1, 2}, 20, {1, 5, 10, 15, 20});
  finalize_block_and_wait_for_beefy(fast_peers, 1, {1});
  finalize_block_and_wait_for_beefy(fast_peers, 6, {5});
  finalize_block_and_wait_for_beefy(fast_peers, 10, {10});
  finalize_block_and_wait_for_beefy(fast_peers, 17, {15});
  // 24 is not checked in polkadot-sdk test,
  // but is justified because 24 - 20 >= min delta
  finalize_block_and_wait_for_beefy(fast_peers, 24, {20, 24});

  // Spawn Dave, they are now way behind voting and can only catch up through
  // on-demand justif sync.
  // then verify Dave catches up through on-demand justification requests.
  auto expect_dave = [&](BlockNumber finalized, BlockNumber expected) {
    finalize({dave}, finalized);
    loop();
    EXPECT_EQ(peers_.at(dave)->beefy_->finalized(), expected);
  };
  expect_dave(1, 1);
  expect_dave(6, 5);
  expect_dave(10, 10);
  expect_dave(17, 15);
  expect_dave(24, 20);

  peers_.at(dave)->listen_ = true;

  // Have the other peers do some gossip so Dave finds out about their progress.
  finalize_block_and_wait_for_beefy(25, {25});
  finalize_block_and_wait_for_beefy(29, {29});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1028
TEST_F(BeefyTest, should_initialize_voter_at_genesis) {
  // polkadot-sdk finalizes blocks 11..13 but doesn't expect them to be
  // justified. Exclude 11..13 justifications using min delta.
  min_delta_ = 4;
  makePeers(1);
  // push 15 blocks with `AuthorityChange` digests every 10 blocks
  generate_blocks_and_sync(15, 10);
  expect(all(), {});
  // Test initialization at session boundary.
  // verify voter initialized with two sessions starting at blocks 1 and 10
  // verify next vote target is mandatory block 1
  finalize_block_and_wait_for_beefy(1, {1});
  finalize_block_and_wait_for_beefy(13, {10});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1070
TEST_F(BeefyTest, should_initialize_voter_at_custom_genesis) {
  makePeers(1);
  generate_blocks_and_sync(25, 10);
  genesis_ = 15;
  finalize_block_and_wait_for_beefy(genesis_, {genesis_});
  // must ignore mandatory block 20 before new genesis
  genesis_ = 25;
  finalize_block_and_wait_for_beefy(genesis_, {genesis_});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1145
TEST_F(BeefyTest, should_initialize_voter_when_last_final_is_session_boundary) {
  min_delta_ = 4;
  makePeers(1);
  auto &peer = peers_.at(0);

  // push 15 blocks with `AuthorityChange` digests every 10 blocks
  generate_blocks_and_sync(15, 10);

  // finalize 13 without justifications
  // import/append BEEFY justification for session boundary block 10
  finalize_block_and_wait_for_beefy(13, {1, 10});

  // Test corner-case where session boundary == last beefy finalized,
  // expect rounds initialized at last beefy finalized 10.
  // load persistent state - nothing in DB, should init at session boundary
  // verify voter initialized with single session starting at block 10
  // verify block 10 is correctly marked as finalized
  // verify next vote target is diff-power-of-two block 14
  // verify state also saved to db
  reloadPeer(*peer);
  loop();
  EXPECT_EQ(peer->beefy_->finalized(), 10);
  finalize_block_and_wait_for_beefy(14, {14});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1202
TEST_F(BeefyTest, should_initialize_voter_at_latest_finalized) {
  genesis_ = 12;
  min_delta_ = 2;
  makePeers(1);
  auto &peer = peers_.at(0);

  // push 15 blocks with `AuthorityChange` digests every 10 blocks
  generate_blocks_and_sync(15, 10);

  // finalize 13 without justifications
  // import/append BEEFY justification for block 12
  finalize_block_and_wait_for_beefy(13, {12});

  // Test initialization at last BEEFY finalized.
  // load persistent state - nothing in DB, should init at last BEEFY finalized
  // verify voter initialized with single session starting at block 12
  // verify next vote target is 14
  // verify state also saved to db
  reloadPeer(*peer);
  loop();
  EXPECT_EQ(peer->beefy_->finalized(), 12);
  finalize_block_and_wait_for_beefy(14, {14});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1375
TEST_F(BeefyTest, beefy_finalizing_after_pallet_genesis) {
  genesis_ = 15;
  makePeers(2);
  // push 42 blocks including `AuthorityChange` digests every 10 blocks.
  generate_blocks_and_sync(42, 10);
  // GRANDPA finalize blocks leading up to BEEFY pallet genesis -> BEEFY should
  // finalize nothing.
  finalize_block_and_wait_for_beefy(14, {});
  // GRANDPA finalize block #16 -> BEEFY should finalize #15 (genesis mandatory)
  // and #16.
  finalize_block_and_wait_for_beefy(16, {15, 16});
  // GRANDPA finalize #21 -> BEEFY finalize #20 (mandatory) and #21
  finalize_block_and_wait_for_beefy(21, {20, 21});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1308
TEST_F(BeefyTest, should_catch_up_when_loading_saved_voter_state) {
  min_delta_ = 10;
  makePeers(1);
  auto &peer = peers_.at(0);

  // push 30 blocks with `AuthorityChange` digests every 10 blocks
  generate_blocks_and_sync(30, 10);
  // finalize 13 without justifications
  // load persistent state - nothing in DB, should init at genesis
  // Test initialization at session boundary.
  // verify voter initialized with two sessions starting at blocks 1 and 10
  // verify next vote target is mandatory block 1
  finalize_block_and_wait_for_beefy(13, {1, 10});

  // verify state also saved to db
  // now let's consider that the node goes offline, and then it restarts after a
  // while finalize 25 without justifications load persistent state - state
  // preset in DB
  // Verify voter initialized with old sessions plus a new one starting at
  // block 20. There shouldn't be any duplicates.
  reloadPeer(*peer);
  loop();
  EXPECT_EQ(peer->beefy_->finalized(), 10);
  finalize_block_and_wait_for_beefy(25, {20});

  // will duplicate justification without persisted state
  peer->db_ = std::make_shared<InMemorySpacedStorage>();
  reloadPeer(*peer);
  loop();
  expect(all(), {1, 10, 20});
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1409
TEST_F(BeefyTest, beefy_reports_equivocations) {
  makePeers(2);
  generate_blocks_and_sync(1, 10);
  size_t alice = 0, bob = 1;

  // bob signs vote
  peers_.at(alice)->vote_ = false;
  finalize_block_and_wait_for_beefy(1, {});

  // generate duplicate bob vote with different payload
  auto vote = signVote(*peers_.at(bob), {{}, 1, 0});
  peers_.at(alice)->beefy_->onMessage(vote);

  // expect equivocation report
  EXPECT_CALL(*beefy_api_, generate_key_ownership_proof(_, _, _))
      .WillOnce(Return(OpaqueKeyOwnershipProof{}));
  EXPECT_CALL(*offchain_worker_factory_, make()).WillOnce(Return(nullptr));
  EXPECT_CALL(*offchain_worker_pool_, addWorker(_)).WillOnce(Return());
  EXPECT_CALL(*offchain_worker_pool_, removeWorker()).WillOnce(Return(true));
  EXPECT_CALL(*beefy_api_,
              submit_report_double_voting_unsigned_extrinsic(_, _, _))
      .WillOnce(Return(outcome::success()));
  loop();
}

// https://github.com/paritytech/polkadot-sdk/blob/1b76f99e12e9751703417fdb58097a1860aa20b7/substrate/client/consensus/beefy/src/tests.rs#L1481
TEST_F(BeefyTest, gossipped_finality_proofs) {
  makePeers(2);
  generate_blocks_and_sync(42, 10);
  size_t alice = 0, charlie = 1;

  // Only Alice is running the voter -> finality threshold not reached
  // Charlie will run just the gossip engine and not the full voter.
  // Alice runs full voter.
  peers_.at(charlie)->vote_ = false;

  // Alice and Charlie finalize #1, Alice votes on it, but not Charlie.
  // verify nothing gets finalized by BEEFY
  finalize_block_and_wait_for_beefy(1, {});

  // Charlie gossips finality proof for #1 -> Alice and Bob also finalize.
  peers_.at(charlie)->vote_ = true;
  finalize_block_and_wait_for_beefy({charlie}, 1, {1});
  expect({alice}, {});
  // Expect #1 is finalized.
  EXPECT_EQ(peers_.at(alice)->beefy_->finalized(), 1);

  // Code above verifies gossipped finality proofs are correctly imported and
  // consumed by voters. Next, let's verify finality proofs are correctly
  // generated and gossipped by voters.

  // Everyone finalizes #2
  peers_.at(alice)->listen_ = false;
  peers_.at(charlie)->listen_ = false;
  finalize_block_and_wait_for_beefy(2, {});

  // Simulate Charlie vote on #2
  peers_.at(alice)->listen_ = true;
  peers_.at(charlie)->listen_ = true;
  rebroadcast();
  loop();
  // Expect #2 is finalized.
  // Now verify Charlie also sees the gossipped proof generated by either Alice.
  expect(all(), {2});
}
