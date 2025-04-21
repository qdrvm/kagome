/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include <memory>

#include "network/req_pov_observer.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::network {

  class ReqPovProtocol : public ProtocolBase {
   public:
    virtual void request(const PeerId &peer_id,
                         RequestPov,
                         std::function<void(outcome::result<ResponsePov>)>
                             &&response_handler) = 0;
  };

}  // namespace kagome::network
