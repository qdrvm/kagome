/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
