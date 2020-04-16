/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
