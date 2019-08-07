/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_context.hpp>
#include "mock/core/authorship/proposer_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/babe_lottery_mock.hpp"
#include "mock/core/consensus/consensus_network_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "testutil/sr25519_utils.hpp"

using namespace kagome;
using namespace consensus;
using namespace blockchain;
using namespace authorship;
using namespace crypto;
using namespace primitives;
using namespace clock;

class BabeTest : public testing::Test {
 public:
  void SetUp() override {
    generateKeypair(keypair_);
  }

  boost::asio::io_context io_context_;

  std::shared_ptr<BabeLotteryMock> lottery_ =
      std::make_shared<BabeLotteryMock>();
  std::shared_ptr<ProposerMock> proposer_ = std::make_shared<ProposerMock>();
  std::shared_ptr<BlockTreeMock> block_tree_ =
      std::make_shared<BlockTreeMock>();
  std::shared_ptr<ConsensusNetworkMock> network_ =
      std::make_shared<ConsensusNetworkMock>();
  SR25519Keypair keypair_;
  AuthorityIndex authority_id_ = 1;
  std::shared_ptr<SystemClockMock> clock_ = std::make_shared<SystemClockMock>();
  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();
  libp2p::event::Bus event_bus_;

  BabeImpl babe_{
      lottery_,
      proposer_,
      block_tree_,
      network_,
      keypair_,
      authority_id_,
      clock_,
      hasher_,
      boost::asio::basic_waitable_timer<std::chrono::system_clock>{io_context_},
      event_bus_};
};

/**
 * @given BABE production
 * @when running it in epoch with two slots @and out node is a leader in one of
 * them
 * @then block is emitted in the leader slot @and after two slots BABE moves to
 * the next epoch
 */
TEST_F(BabeTest, Success) {}

/**
 * @given BABE production, which is configured to the already finished slot in
 * the current epoch
 * @when launching it
 * @then it synchronizes successfully
 */
TEST_F(BabeTest, SyncSuccess) {}

/**
 * @given BABE production, which is configured to the already finished slot in
 * the previous epoch
 * @when launching it
 * @then it fails to synchronize
 */
TEST_F(BabeTest, BigDelay) {}
