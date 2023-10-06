/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REQ_POV_OBSERVER_HPP
#define KAGOME_REQ_POV_OBSERVER_HPP

#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to request PoV protocol
   */
  class ReqPovObserver {
   public:
    virtual ~ReqPovObserver() = default;

    /**
     * We should response with PoV block if we seconded this candidate
     */
    virtual outcome::result<ResponsePov> OnPovRequest(RequestPov request) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_REQ_POV_OBSERVER_HPP
