/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/multi/converters/converter_utils.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"
extern "C" {
#include "libp2p/multi/c-utils/protoutils.h"
}
using libp2p::multi::converters::addressToBytes;
using libp2p::multi::ProtocolList;
using kagome::common::Buffer;


TEST(AddressConverter, Ip4AddressToBytes) {
  auto bytes = addressToBytes(*ProtocolList::get("ip4"), "127.0.0.1").value();
  Buffer b {127, 0, 0, 1};
  ASSERT_EQ(bytes, "7F000001");

  auto p = *ProtocolList::get("ip4");
  struct Protocol cp;
  cp.deccode = static_cast<int>(p.code);
  cp.size = p.size;
  memcpy(cp.name, p.name.data(), p.name.size());
  char* result = (char*)malloc(256);
  int s = 256;
  address_string_to_bytes(&cp, "127.0.0.1", 9, &result, &s);
  result[s] = '\0';

  ASSERT_EQ(bytes, result);
}

TEST(AddressConverter, TcpAddressToBytes) {
  auto bytes = addressToBytes(*ProtocolList::get(libp2p::multi::Protocol::Code::tcp), "443").value();

  auto p = *ProtocolList::get("tcp");
  struct Protocol cp;
  cp.deccode = static_cast<int>(p.code);
  cp.size = p.size;
  memcpy(cp.name, p.name.data(), p.name.size());
  char* result = (char*)malloc(256);
  int s = 256;
  address_string_to_bytes(&cp, "443", 3, &result, &s);

  ASSERT_EQ(bytes, result);
}

TEST(AddressConverter, UdpAddressToBytes) {
  auto bytes = addressToBytes(*ProtocolList::get(libp2p::multi::Protocol::Code::udp), "4242").value();

  auto p = *ProtocolList::get("udp");
  struct Protocol cp;
  cp.deccode = 17;
  cp.size = p.size;
  memcpy(cp.name, p.name.data(), p.name.size());
  char* result = (char*)malloc(256);
  int s = 256;
  address_string_to_bytes(&cp, "4242", 4, &result, &s);

  ASSERT_EQ(bytes, result);
}

TEST(AddressConverter, IpfsAddressToBytes) {
  auto bytes = addressToBytes(*ProtocolList::get(libp2p::multi::Protocol::Code::ipfs), "QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG").value();

  auto p = *ProtocolList::get("ipfs");
  struct Protocol cp;
  cp.deccode = 42;
  cp.size = p.size;
  memcpy(cp.name, p.name.data(), p.name.size());
  char* result = (char*)malloc(256);
  int sz = 256;
  const char* s = "QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG";
  char* r = address_string_to_bytes(&cp, s, strlen(s), &result, &sz);
  ASSERT_STRNE(r, "ERR");

  ASSERT_EQ(bytes, result);
}
