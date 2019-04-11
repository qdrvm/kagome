/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/multi/converters/converter_utils.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

using kagome::common::Buffer;
using libp2p::multi::ProtocolList;
using libp2p::multi::converters::addressToHex;

/**
 * @given A string with an ip4 address
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, Ip4AddressToBytes) {
  auto bytes = addressToHex(*ProtocolList::get("ip4"), "127.0.0.1").value();
  ASSERT_EQ(bytes, "7F000001");

  bytes = addressToHex(*ProtocolList::get("ip4"), "0.0.0.1").value();
  ASSERT_EQ(bytes, "00000001");
}

/**
 * @given A string with a tcp address (a port, actually)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, TcpAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::tcp);
  ASSERT_EQ("04D2", addressToHex(*p, "1234").value());
  ASSERT_EQ("0000", addressToHex(*p, "0").value());
}

/**
 * @given A string with a udp address (a port, actually)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, UdpAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::udp);
  ASSERT_EQ("04D2", addressToHex(*p, "1234").value());
  ASSERT_EQ("0000", addressToHex(*p, "0").value());
}

/**
 * @given A string with an ipfs address (base58 encoded)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, IpfsAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::ipfs);
  ASSERT_EQ(
      "221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B",
      addressToHex(*p, "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC")
          .value());
}
