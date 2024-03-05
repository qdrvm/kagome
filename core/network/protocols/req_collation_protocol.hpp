/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include <memory>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/req_collation_observer.hpp"
#include "network/types/collator_messages_vstaging.hpp"

namespace kagome::network {

  class IReqCollationProtocol : public ProtocolBase {
   public:
    virtual void request(
        const PeerId &peer_id,
        CollationFetchingRequest request,
        std::function<void(outcome::result<CollationFetchingResponse>)>
            &&response_handler) = 0;

    virtual void request(
        const PeerId &peer_id,
        vstaging::CollationFetchingRequest request,
        std::function<
            void(outcome::result<vstaging::CollationFetchingResponse>)>
            &&response_handler) = 0;
  };

}  // namespace kagome::network
