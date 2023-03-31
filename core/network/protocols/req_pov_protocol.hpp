/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REQ_POV_PROTOCOL_HPP
#define KAGOME_REQ_POV_PROTOCOL_HPP

#include "network/protocol_base.hpp"

#include <memory>

#include "network/req_pov_observer.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::network {

  class IReqPovProtocol : public ProtocolBase {
   public:
    virtual void request(const PeerInfo &peer_info,
                         RequestPov,
                         std::function<void(outcome::result<ResponsePov>)>
                             &&response_handler) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_REQ_POV_PROTOCOL_HPP
