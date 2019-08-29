/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_CLIENT_HPP
#define KAGOME_GOSSIPER_CLIENT_HPP

#include "network/gossiper.hpp"

namespace kagome::network {
  /**
   * "Active" part of consensus gossiping, representing one client (or node, or
   * peer in another words)
   */
  struct GossiperClient : public Gossiper {};
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_CLIENT_HPP
