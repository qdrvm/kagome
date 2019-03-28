/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string_view>

#include <gtest/gtest.h>
#include "libp2p/peer/peer_info.hpp"

using namespace libp2p::multi;
using namespace libp2p::peer;
using namespace kagome::common;

class PeerInfoTest : public ::testing::Test {
 public:
  /// must be a SHA-256 hash
  Multihash valid_peer_id =
      Multihash::create(HashType::sha256, Buffer{0xAA, 0xBB}).getValue();
  Multihash invalid_peer_id =
      Multihash::create(HashType::blake2s128, Buffer{0xAA, 0xBB}).getValue();

  std::vector<Multiaddress::Protocol> protocols{Multiaddress::Protocol::kDccp,
                                                Multiaddress::Protocol::kIp4};

  std::vector<Multiaddress> addresses{
      Multiaddress::create("/ip4/192.168.0.1/udp/228/").value(),
      Multiaddress::create("/ip4/192.168.0.1/udp/").value()};

  /**
   * Create a valid PeerInfo
   * @return PeerInfo
   */
  PeerInfo createValid() {
    return PeerInfo::createPeerInfo(valid_peer_id).value();
  }
};

/**
 * @given valid PeerId multihash
 * @when initializing PeerInfo from that multihash
 * @then initialization succeds
 */
TEST_F(PeerInfoTest, CreateSuccess) {
  auto peer_info = PeerInfo::createPeerInfo(valid_peer_id);

  ASSERT_TRUE(peer_info);
  ASSERT_EQ(peer_info.value().peerId(), valid_peer_id);
}

/**
 * @given invalid PeerId multihash
 * @when initializing PeerInfo from that multihash
 * @then initialization fails
 */
TEST_F(PeerInfoTest, CreateInvalidId) {
  auto peer_info = PeerInfo::createPeerInfo(invalid_peer_id);

  ASSERT_FALSE(peer_info);
}

/**
 * @given initialized PeerInfo
 * @when adding protocols to this PeerInfo
 * @then protocols are added
 */
TEST_F(PeerInfoTest, AddProtocols) {
  auto peer_info = createValid();
  peer_info.addProtocols(protocols);

  const auto &added_protocols = peer_info.supportedProtocols();
  ASSERT_EQ(added_protocols.size(), 2);
  for (const auto &protocol : protocols) {
    ASSERT_NE(added_protocols.find(protocol), added_protocols.end());
  }
}

/**
 * @given initialized PeerInfo with some protocols
 * @when removing protocol from this PeerInfo
 * @then protocol is removed
 */
TEST_F(PeerInfoTest, RemoveProtocolSuccess) {
  auto peer_info = createValid();
  peer_info.addProtocols(protocols);

  const auto &added_protocols = peer_info.supportedProtocols();
  ASSERT_EQ(added_protocols.size(), 2);
  ASSERT_NE(added_protocols.find(protocols[0]), added_protocols.end());

  peer_info.removeProtocol(protocols[0]);

  ASSERT_EQ(added_protocols.size(), 1);
  ASSERT_EQ(added_protocols.find(protocols[0]), added_protocols.end());
}

/**
 * @given initialized PeerInfo with some protocols
 * @when removing protocol, which is not in this PeerInfo
 * @then protocol is not removed
 */
TEST_F(PeerInfoTest, RemoveProtocolFail) {
  auto peer_info = createValid();
  peer_info.addProtocols(protocols);

  ASSERT_FALSE(peer_info.removeProtocol(Multiaddress::Protocol::kIp6));
}

/**
 * @given initialized PeerInfo
 * @when adding addresses, passed via span, to this PeerInfo
 * @then addresses are added
 */
TEST_F(PeerInfoTest, AddAddressesSpan) {
  auto peer_info = createValid();
  peer_info.addMultiaddresses(addresses);

  const auto &added_addresses = peer_info.multiaddresses();
  ASSERT_EQ(added_addresses.size(), 2);
  for (const auto &address : addresses) {
    ASSERT_NE(added_addresses.find(address), added_addresses.end());
  }
}

/**
 * @given initialized PeerInfo
 * @when adding addresses, passed via vector move, to this PeerInfo
 * @then addresses are added
 */
TEST_F(PeerInfoTest, AddAddressesVector) {
  auto peer_info = createValid();
  auto addresses_copy = addresses;
  peer_info.addMultiaddresses(std::move(addresses_copy));

  const auto &added_addresses = peer_info.multiaddresses();
  ASSERT_EQ(added_addresses.size(), 2);
  for (const auto &address : addresses) {
    ASSERT_NE(added_addresses.find(address), added_addresses.end());
  }
}

/**
 * @given initialized PeerInfo with some addresses
 * @when removing address from this PeerInfo
 * @then address is removed
 */
TEST_F(PeerInfoTest, RemoveAddressSuccess) {
  auto peer_info = createValid();
  peer_info.addMultiaddresses(addresses);

  const auto &added_addresses = peer_info.multiaddresses();
  ASSERT_EQ(added_addresses.size(), 2);
  ASSERT_NE(added_addresses.find(addresses[0]), added_addresses.end());

  peer_info.removeMultiaddress(addresses[0]);

  ASSERT_EQ(added_addresses.size(), 1);
  ASSERT_EQ(added_addresses.find(addresses[0]), added_addresses.end());
}

/**
 * @given initialized PeerInfo with some addresses
 * @when removing address, which is not in this PeerInfo
 * @then address is not removed
 */
TEST_F(PeerInfoTest, RemoveAddressFail) {
  auto peer_info = createValid();
  peer_info.addMultiaddresses(addresses);

  ASSERT_FALSE(
      peer_info.removeMultiaddress(Multiaddress::create("/ip4/").value()));
}

/**
 * @given initialized PeerInfo
 * @when adding address via safe method
 * @then address is added only after the second attempt
 */
TEST_F(PeerInfoTest, AddAddressSafe) {
  auto peer_info = createValid();
  const auto &added_addresses = peer_info.multiaddresses();

  ASSERT_EQ(added_addresses.size(), 0);
  ASSERT_FALSE(peer_info.addMultiaddressSafe(addresses[0]));
  ASSERT_EQ(added_addresses.size(), 0);
  ASSERT_TRUE(peer_info.addMultiaddressSafe(addresses[0]));
  ASSERT_EQ(added_addresses.size(), 1);
  ASSERT_NE(added_addresses.find(addresses[0]), added_addresses.end());
}

/**
 * @given initialized PeerInfo with some addresses
 * @when replacing those addresses via another ones
 * @then addresses are replaced
 */
TEST_F(PeerInfoTest, ReplaceAddresses) {
  std::vector<Multiaddress> another_addresses{
      Multiaddress::create("/ip4/").value(),
      Multiaddress::create("/ip4/192.168.0.1/").value()};
  auto peer_info = createValid();
  const auto &added_addresses = peer_info.multiaddresses();

  peer_info.addMultiaddresses(addresses);
  ASSERT_EQ(added_addresses.size(), 2);
  for (const auto &address : addresses) {
    ASSERT_NE(added_addresses.find(address), added_addresses.end());
  }

  peer_info.replaceMultiaddresses(addresses, another_addresses);
  ASSERT_EQ(added_addresses.size(), 2);
  for (const auto &address : another_addresses) {
    ASSERT_NE(added_addresses.find(address), added_addresses.end());
  }
}
