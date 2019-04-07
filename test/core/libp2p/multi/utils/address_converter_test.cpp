/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/multi/converters/converter_utils.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

using libp2p::multi::converters::addressToBytes;
using libp2p::multi::ProtocolList;
using kagome::common::Buffer;


TEST(AddressConverter, Ip4AddressToBytes) {
  auto bytes = addressToBytes(*ProtocolList::get("ip4"), "127.0.0.1").value();
  ASSERT_EQ(bytes, "7F000001");

  bytes = addressToBytes(*ProtocolList::get("ip4"), "0.0.0.1").value();
  ASSERT_EQ(bytes, "00000001");
}

TEST(AddressConverter, TcpAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::tcp);
  ASSERT_EQ("04D2", addressToBytes(*p, "1234").value());
  ASSERT_EQ("0000", addressToBytes(*p, "0").value());
}

TEST(AddressConverter, UdpAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::udp);
  ASSERT_EQ("04D2", addressToBytes(*p, "1234").value());
  ASSERT_EQ("0000", addressToBytes(*p, "0").value());
}

TEST(AddressConverter, IpfsAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::ipfs);
  ASSERT_EQ("221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B", addressToBytes(*p, "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC").value());
}
