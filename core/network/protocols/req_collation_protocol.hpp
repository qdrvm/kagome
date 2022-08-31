/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REQCOLLATIONROTOCOL
#define KAGOME_NETWORK_REQCOLLATIONROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/req_collation_observer.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::network {

  class IReqCollationProtocol : public ProtocolBase {
   public:
    virtual void request(
        const PeerId &peer_id,
        CollationFetchingRequest request,
        std::function<void(outcome::result<CollationFetchingResponse>)>
            &&response_handler) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REQCOLLATIONROTOCOL
