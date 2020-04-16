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

#ifndef KAGOME_BABE_OBSERVER_HPP
#define KAGOME_BABE_OBSERVER_HPP

#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to BABE
   */
  struct BabeObserver {
    virtual ~BabeObserver() = default;

    /**
     * Triggered when a BlockAnnounce message arrives
     * @param announce - arrived message
     */
    virtual void onBlockAnnounce(const BlockAnnounce &announce) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_BABE_OBSERVER_HPP
