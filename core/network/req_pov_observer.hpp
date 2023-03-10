/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REQ_POV_OBSERVER_HPP
#define KAGOME_REQ_POV_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "consensus/grandpa/common.hpp"
#include "network/types/collator_messages.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to request PoV protocol
   */
  struct ReqPovObserver {
    virtual ~ReqPovObserver() = default;

    /**
     * We should response with PoV block if we seconded this candidate
     */
    virtual outcome::result<ResponsePov> OnPovRequest(RequestPov request) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_REQ_POV_OBSERVER_HPP
