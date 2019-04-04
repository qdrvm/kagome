/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/multi/utils/protocol_list.hpp"

using libp2p::multi::ProtocolList;
using libp2p::multi::Protocol;

TEST(ProtocolList, getByName) {
  static_assert(ProtocolList::get("ip4")->name == "ip4");
  static_assert(!ProtocolList::get("ip5"));
}

TEST(ProtocolList, getByCode) {
  static_assert(ProtocolList::get(Protocol::Code::ip6)->name == "ip6");
  static_assert(ProtocolList::get(Protocol::Code::dccp)->dec_code == Protocol::Code::dccp);
  static_assert(!ProtocolList::get(static_cast<Protocol::Code>(1312312)));
}
