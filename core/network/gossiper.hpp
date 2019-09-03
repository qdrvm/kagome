/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_HPP
#define KAGOME_GOSSIPER_HPP

#include "consensus/grandpa/gossiper.hpp"
#include "network/babe_gossiper.hpp"

namespace kagome::network {
  /**
   * Joins all available gossipers
   */
  struct Gossiper : public BabeGossiper, public consensus::grandpa::Gossiper {};
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_HPP
