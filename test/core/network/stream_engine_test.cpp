/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/stream_engine.hpp"

#include <gtest/gtest.h>
#include <gsl/span>

#include "mock/core/network/protocols/state_protocol_mock.hpp"
#include "mock/core/network/protocols/sync_protocol_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::network::StreamEngine;
using libp2p::connection::StreamMock;
using libp2p::peer::PeerId;

using testing::_;
using testing::Between;
using testing::InvokeArgument;
using testing::Return;

namespace kagome::network {
  struct StreamEngineTest : ::testing::Test {
   public:
    static void SetUpTestCase() {
      testutil::prepareLoggers();
    }
  };

  static constexpr int lucky_peers = 4;

  /// Mock PRNG that produces a ring sequence of [0-9]
  struct RngMock {
    using result_type = uint32_t;
    static constexpr int max_rng_val = 9;

    result_type operator()() {
      static result_type value{9};
      value = ++value % 10;
      return value;
    }

    static result_type max() {
      return max_rng_val;
    }
  };

  /**
   * @given mock PRNG with known sequence, number of candidate peers, number of
   * "lucky" peers, two protocols p1 and p2 and StreamEngine with 20 streams,
   * each on distinct host, so that we have equal number of
   * combinations [p1/p2]/[in/out]
   * @when executing broadcast with RandomGossipStrategy over StreamEngine
   * @then we will send to expected number of peers (exactly lucky_peers in our
   * case)
   */
  TEST_F(StreamEngineTest, RandomGossipTest) {
    // threshold = max_val * max(lucky_peers / candidates, 1.0)
    StreamEngine stream_engine;
    std::shared_ptr<ProtocolBase> protocol1 =
        std::make_shared<StateProtocolMock>();
    std::shared_ptr<ProtocolBase> protocol2 =
        std::make_shared<SyncProtocolMock>();
    std::vector<PeerId> peer_ids{
        "peer00"_peerid, "peer01"_peerid, "peer02"_peerid, "peer03"_peerid,
        "peer04"_peerid, "peer05"_peerid, "peer06"_peerid, "peer07"_peerid,
        "peer08"_peerid, "peer09"_peerid, "peer10"_peerid, "peer11"_peerid,
        "peer12"_peerid, "peer13"_peerid, "peer14"_peerid, "peer15"_peerid,
        "peer16"_peerid, "peer17"_peerid, "peer18"_peerid, "peer19"_peerid,
    };

    size_t counter{0};
    for (size_t i = 0; i < peer_ids.size(); ++i) {
      auto stream = std::make_shared<StreamMock>();
      EXPECT_CALL(*stream, remotePeerId()).WillOnce(Return(peer_ids.at(i)));
      EXPECT_CALL(*stream, write(_, _, _)).WillRepeatedly([&counter]() {
        ++counter;
      });
      if (i % 2 == 0) {
        EXPECT_OUTCOME_TRUE_1(stream_engine.addIncoming(
            std::move(stream), i % 4 < 2 ? protocol1 : protocol2));
      } else {
        EXPECT_OUTCOME_TRUE_1(stream_engine.addOutgoing(
            std::move(stream), i % 4 < 2 ? protocol1 : protocol2));
      }
    }

    auto peers_num = stream_engine.outgoingStreamsNumber(protocol1);
    ASSERT_EQ(peers_num, 5);
    auto gossip_strategy =
        StreamEngine::RandomGossipStrategy<RngMock>{peers_num, lucky_peers};

    auto msg = std::make_shared<int>(42);
    stream_engine.broadcast<int>(protocol1, msg, gossip_strategy);
    ASSERT_EQ(counter, lucky_peers);
  }
}  // namespace kagome::network