/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/basic/scheduler.hpp>

#include "common/main_thread_pool.hpp"
#include "consensus/beefy/impl/beefy_impl.hpp"
#include "consensus/beefy/impl/beefy_thread_pool.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/application/chain_spec_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/timeline/timeline_mock.hpp"
#include "mock/core/crypto/session_keys_mock.hpp"
#include "mock/core/network/protocols/beefy_protocol_mock.hpp"
#include "mock/core/runtime/beefy_api.hpp"
#include "network/impl/protocols/beefy_protocol_impl.hpp"
#include "primitives/event_types.hpp"
#include "storage/in_memory/in_memory_spaced_storage.hpp"
#include "testutil/lazy.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::TestThreadPool;
using kagome::application::AppStateManagerMock;
using kagome::application::ChainSpecMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::MainPoolHandler;
using kagome::common::MainThreadPool;
using kagome::consensus::Timeline;
using kagome::consensus::TimelineMock;
using kagome::consensus::beefy::BeefyGossipMessage;
using kagome::consensus::beefy::BeefyJustification;
using kagome::consensus::beefy::ConsensusDigest;
using kagome::consensus::beefy::MmrRootHash;
using kagome::consensus::beefy::SignedCommitment;
using kagome::consensus::beefy::ValidatorSet;
using kagome::crypto::EcdsaKeypair;
using kagome::crypto::EcdsaProviderImpl;
using kagome::crypto::EcdsaSeed;
using kagome::crypto::HasherImpl;
using kagome::crypto::SessionKeysMock;
using kagome::network::BeefyImpl;
using kagome::network::BeefyProtocol;
using kagome::network::BeefyProtocolMock;
using kagome::network::BeefyThreadPool;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::primitives::Consensus;
using kagome::primitives::kBeefyEngineId;
using kagome::primitives::events::ChainEventType;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::ChainSubscriptionEnginePtr;
using kagome::runtime::BeefyApiMock;
using kagome::storage::InMemorySpacedStorage;
using testing::_;
using testing::Return;

struct Timer : libp2p::basic::Scheduler {
  void pulse(std::chrono::milliseconds current_clock) noexcept override {
    abort();
  }
  std::chrono::milliseconds now() const noexcept override {
    abort();
  }
  Handle scheduleImpl(Callback &&cb,
                      std::chrono::milliseconds,
                      bool) noexcept override {
    cb_.emplace(std::move(cb));
    return Handle{};
  }
  void cancel(Handle::Ticket ticket) noexcept override {
    abort();
  }
  outcome::result<Handle::Ticket> reschedule(
      Handle::Ticket ticket,
      std::chrono::milliseconds delay_from_now) noexcept override {
    abort();
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
  EcdsaKeypair keys_;
  BlockNumber finalized_ = 0;
  std::vector<BlockNumber> justifications_;

  std::shared_ptr<BlockTreeMock> block_tree_ =
      std::make_shared<BlockTreeMock>();
  std::shared_ptr<Timer> timer_ = std::make_shared<Timer>();
  std::shared_ptr<SessionKeysMock> keystore_ =
      std::make_shared<SessionKeysMock>();
  std::shared_ptr<BeefyProtocolMock> broadcast_ =
      std::make_shared<BeefyProtocolMock>();
  ChainSubscriptionEnginePtr chain_sub_ =
      std::make_shared<ChainSubscriptionEngine>();
  std::shared_ptr<BeefyImpl> beefy_;
};

struct BeefyTest : testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(chain_spec_, beefyMinDelta()).WillRepeatedly([&] {
      return min_delta_;
    });
    EXPECT_CALL(*beefy_api_, genesis(_)).WillRepeatedly([&] {
      return genesis_;
    });
    EXPECT_CALL(*beefy_api_, validatorSet(_)).WillRepeatedly([&] {
      return genesisVoters();
    });
    EXPECT_CALL(*timeline_, wasSynchronized()).WillRepeatedly(Return(true));
  }

  void makePeers(uint32_t n) {
    peers_.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
      auto &peer = peers_.emplace_back();
      EcdsaSeed seed;
      seed[0] = i;
      seed[1] = 1;
      peer.keys_ = ecdsa_->generateKeypair(seed, {}).value();
      EXPECT_CALL(*peer.keystore_, getBeefKeyPair(_))
          .WillRepeatedly(Return(
              std::make_pair(std::make_shared<EcdsaKeypair>(peer.keys_), i)));

      std::shared_ptr<AppStateManagerMock> app_state_manager_ =
          std::make_shared<AppStateManagerMock>();
      std::shared_ptr<MainThreadPool> main_thread_pool_ =
          std::make_shared<MainThreadPool>(TestThreadPool{io_});
      std::shared_ptr<MainPoolHandler> main_pool_handler_ =
          std::make_shared<MainPoolHandler>(app_state_manager_,
                                            main_thread_pool_);
      main_pool_handler_->start();
      std::shared_ptr<BeefyThreadPool> beefy_thread_pool_ =
          std::make_shared<BeefyThreadPool>(TestThreadPool{io_});

      EXPECT_CALL(*app_state_manager_, atLaunch(_));

      EXPECT_CALL(*peer.block_tree_, getLastFinalized()).WillRepeatedly([&] {
        return blocks_[peer.finalized_].blockInfo();
      });
      EXPECT_CALL(*peer.block_tree_, getBlockHash(_))
          .WillRepeatedly([&](BlockNumber i) { return blocks_[i].hash(); });
      EXPECT_CALL(*peer.block_tree_, getBlockHeader(_))
          .WillRepeatedly([&](BlockHash h) {
            for (auto &block : blocks_) {
              if (block.hash() == h) {
                return block;
              }
            }
            throw std::logic_error{"getBlockHeader"};
          });
      EXPECT_CALL(*peer.broadcast_, broadcast(_))
          .WillRepeatedly([&](std::shared_ptr<BeefyGossipMessage> m) {
            if (auto jr = boost::get<BeefyJustification>(&*m)) {
              auto &j = boost::get<SignedCommitment>(*jr);
              peer.justifications_.emplace_back(j.commitment.block_number);
            }
            io_->post([&, m] {
              for (auto &peer2 : peers_) {
                if (&peer2 != &peer) {
                  peer2.beefy_->onMessage(*m);
                }
              }
            });
          });
      peer.beefy_ = std::make_shared<BeefyImpl>(
          *app_state_manager_,
          chain_spec_,
          peer.block_tree_,
          beefy_api_,
          ecdsa_,
          std::make_shared<InMemorySpacedStorage>(),
          main_pool_handler_,
          beefy_thread_pool_,
          peer.timer_,
          testutil::sptr_to_lazy<Timeline>(timeline_),
          peer.keystore_,
          testutil::sptr_to_lazy<BeefyProtocol>(peer.broadcast_),
          peer.chain_sub_);
      peer.beefy_->prepare();
      peer.beefy_->start();
    }
  }

  ValidatorSet genesisVoters() {
    ValidatorSet voters;
    for (auto &peer : peers_) {
      voters.validators.emplace_back(peer.keys_.public_key);
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
      BlockNumber genesis = 0;
      if (i > genesis and i % session == 0) {
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
      peers_[i].finalized_ = finalized;
      peers_[i].chain_sub_->notify(ChainEventType::kFinalizedHeads,
                                   blocks_.at(finalized));
    }
  }

  void loop() {
    io_->restart();
    io_->run();
  }

  void expect(std::set<size_t> peers, std::vector<BlockNumber> expected) {
    for (auto &i : peers) {
      EXPECT_EQ(peers_[i].justifications_, expected);
      peers_[i].justifications_.clear();
    }
  }

  void rebroadcast() {
    for (auto &peer : peers_) {
      peer.timer_->call();
    }
  }

  void finalize_block_and_wait_for_beefy(BlockNumber finalized,
                                         std::vector<BlockNumber> expected) {
    finalize(all(), finalized);
    loop();
    expect(all(), expected);
  }

  ChainSpecMock chain_spec_;
  std::shared_ptr<BeefyApiMock> beefy_api_ = std::make_shared<BeefyApiMock>();
  std::shared_ptr<HasherImpl> hasher_ = std::make_shared<HasherImpl>();
  std::shared_ptr<EcdsaProviderImpl> ecdsa_ =
      std::make_shared<EcdsaProviderImpl>(hasher_);
  std::shared_ptr<boost::asio::io_context> io_ =
      std::make_shared<boost::asio::io_context>();
  std::shared_ptr<TimelineMock> timeline_ = std::make_shared<TimelineMock>();

  std::vector<BlockHeader> blocks_;
  BlockNumber min_delta_ = 1;
  BlockNumber genesis_ = 1;
  std::vector<BeefyPeer> peers_;
};

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

TEST_F(BeefyTest, lagging_validators) {
  min_delta_ = 1;
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
