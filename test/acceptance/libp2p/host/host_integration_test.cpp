/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host/basic_host/basic_host.hpp"

#include <chrono>
#include <future>

#include <gtest/gtest.h>
#include "acceptance/libp2p/host/peer/test_peer.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;

using ::testing::_;
using ::testing::Return;

using std::chrono_literals::operator""s;

struct HostIntegrationTest
    : public ::testing::TestWithParam<std::tuple<
          size_t, size_t, uint16_t, Peer::Duration, Peer::Duration>> {
  template <class T>
  using sptr = std::shared_ptr<T>;

  using Duration = kagome::clock::SteadyClockImpl::Duration;

  using PeerPromise = std::promise<peer::PeerInfo>;
  using PeerFuture = std::shared_future<peer::PeerInfo>;

  std::vector<sptr<Peer>> peers;
  std::vector<multi::Multiaddress> addresses;
  std::vector<PeerPromise> peerinfo_promises;
  std::vector<PeerFuture> peerinfo_futures;
};

/**
 * @given a predefined number of peers each represents an echo server
 * @when each peer starts its server, obtains peerinfo
 * @and sets value to peerinfo promises
 * @and initiates client sessions to all other servers
 * @then all clients interact with all servers predefined number of times
 */
TEST_P(HostIntegrationTest, InteractAllToAllSuccess) {
  const auto [peer_count, ping_times, start_port, timeout, future_timeout] =
      GetParam();
  const auto addr_prefix = "/ip4/127.0.0.1/tcp/";

  // initialize
  peers.reserve(peer_count);
  for (size_t i = 0; i < peer_count; ++i) {
    peers.push_back(std::make_shared<Peer>(timeout));
  }

  addresses.reserve(peer_count);
  peerinfo_promises.reserve(peer_count);
  peerinfo_futures.reserve(peer_count);

  // initialize peerinfo promises and futures and addresses
  for (size_t i = 0; i < peer_count; ++i) {
    auto port = i + start_port;
    auto addr = addr_prefix + std::to_string(port);
    EXPECT_OUTCOME_TRUE(ma, multi::Multiaddress::create(addr));
    addresses.push_back(std::move(ma));
    PeerPromise promise{};
    PeerFuture future = promise.get_future();
    peerinfo_promises.push_back(std::move(promise));
    peerinfo_futures.push_back(std::move(future));
  }

  // start servers
  for (size_t i = 0; i < peer_count; ++i) {
    auto &peer = peers[i];
    auto &address = addresses[i];
    auto &promise = peerinfo_promises[i];
    peer->startServer(address, promise);
  }

  // need to wait for peerinfo values before starting client sessions
  for (auto &f : peerinfo_futures) {
    f.wait_for(future_timeout);
  }

  // start client sessions from all peers to all other peers
  for (size_t i = 0; i < peer_count; ++i) {
    for (size_t j = 0; j < peer_count; ++j) {
      const auto &pinfo = peerinfo_futures[j].get();
      auto checker = std::make_shared<TickCounter>(ping_times);
      peers[i]->startClient(i, pinfo, ping_times, std::move(checker));
    }
  }

  // wait for peers to finish their jobs
  for (const auto &p : peers) {
    p->wait();
  }
}

INSTANTIATE_TEST_CASE_P(AllTestCases, HostIntegrationTest,
                        ::testing::Values(
                            // ports are not freed, so new ports each time
                            std::make_tuple(1u, 1u, 40510u, 2s, 2s),
                            std::make_tuple(11u, 10u, 40511u, 5s, 2s),
                            std::make_tuple(5u, 100u, 40522u, 5s, 2s)));
