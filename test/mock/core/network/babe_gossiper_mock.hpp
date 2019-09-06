/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_GOSSIPER_MOCK_HPP
#define KAGOME_BABE_GOSSIPER_MOCK_HPP

#include "network/babe_gossiper.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  struct BabeGossiperMock : public BabeGossiper {
    MOCK_METHOD1(blockAnnounce, void(const BlockAnnounce &));
  };
}  // namespace kagome::network

#endif  // KAGOME_BABE_GOSSIPER_MOCK_HPP
