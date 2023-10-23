/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/bootstrap_nodes.hpp"

#include <gmock/gmock.h>

#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/chain_spec_mock.hpp"
#include "testutil/literals.hpp"

using namespace kagome::application;
using namespace kagome::network;
using namespace libp2p;
using ::testing::ReturnRef;

struct BootstrapNodesTest : public ::testing::Test {
  void SetUp() override {
    acm_bootnodes.clear();
    scm_bootnodes.clear();
  }

  std::string peerA = "/p2p/" + "AAA"_peerid.toBase58();
  std::string peerB = "/p2p/" + "BBB"_peerid.toBase58();
  std::string peerC = "/p2p/" + "CCC"_peerid.toBase58();

  std::string addr1 = "/ip4/1.1.1.1/tcp/1111";
  std::string addr2 = "/ip4/2.2.2.2/tcp/2222";
  std::string addr3 = "/ip4/3.3.3.3/tcp/3333";
  std::string addr4 = "/ip4/4.4.4.4/tcp/4444";

  std::vector<multi::Multiaddress> acm_bootnodes;
  std::vector<multi::Multiaddress> scm_bootnodes;

  std::unique_ptr<BootstrapNodes> makeBootnodes() {
    AppConfigurationMock acm;
    EXPECT_CALL(acm, bootNodes()).WillOnce(ReturnRef(acm_bootnodes));

    ChainSpecMock csm;
    EXPECT_CALL(csm, bootNodes()).WillOnce(ReturnRef(scm_bootnodes));

    return std::make_unique<BootstrapNodes>(acm, csm);
  }

  void addMultiaddress(std::vector<multi::Multiaddress> &dst,
                       std::string peer,
                       std::string addr) {
    auto ma_res = multi::Multiaddress::create(addr + peer);
    assert(ma_res.has_value());
    auto peer_id_base58_opt = ma_res.value().getPeerId();
    assert(peer_id_base58_opt.has_value());
    dst.emplace_back(std::move(ma_res.value()));
  }
};

/**
 * @given 4 multiadresses: 2 in app_config, 2 in chain_spec (per one address of
 * one peer)
 * @when make bootnodes
 * @then bootnodes contains peer_infos for three unique peers
 */
TEST_F(BootstrapNodesTest, UniquePeers) {
  addMultiaddress(acm_bootnodes, peerA, addr1);
  addMultiaddress(acm_bootnodes, peerB, addr2);
  addMultiaddress(scm_bootnodes, peerB, addr3);
  addMultiaddress(scm_bootnodes, peerC, addr4);

  auto bn = makeBootnodes();

  EXPECT_EQ(bn->size(), 3);
}

/**
 * @given 4 multiadresses of single peer: 2 in app_config, 2 in chain_spec (with
 * one address exists in both sources)
 * @when make bootnodes
 * @then bootnodes contains one peer_info with three unique address
 */
TEST_F(BootstrapNodesTest, UniqueAddrs) {
  addMultiaddress(acm_bootnodes, peerA, addr1);
  addMultiaddress(acm_bootnodes, peerA, addr2);
  addMultiaddress(scm_bootnodes, peerA, addr2);
  addMultiaddress(scm_bootnodes, peerA, addr3);

  auto bn = makeBootnodes();

  EXPECT_EQ(bn->size(), 1);
  EXPECT_EQ(bn->front().addresses.size(), 3);
}

/**
 * @given 4 different multiadresses: 2 in app_config, 2 in chain_spec (with same
 * couple oof peer in both sources)
 * @when make bootnodes
 * @then bootnodes contains two peer_info for each peers with couple unique
 * address
 */
TEST_F(BootstrapNodesTest, Merge) {
  addMultiaddress(acm_bootnodes, peerA, addr1);
  addMultiaddress(acm_bootnodes, peerB, addr2);
  addMultiaddress(scm_bootnodes, peerB, addr3);
  addMultiaddress(scm_bootnodes, peerA, addr4);

  auto bn = makeBootnodes();

  EXPECT_EQ(bn->size(), 2);
  EXPECT_EQ(bn->front().addresses.size(), 2);
  EXPECT_EQ(bn->back().addresses.size(), 2);
}
