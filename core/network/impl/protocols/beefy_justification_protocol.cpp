/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/beefy_justification_protocol.hpp"

#include "consensus/beefy/beefy.hpp"
#include "network/common.hpp"

namespace kagome::network {
  BeefyJustificationProtocol::BeefyJustificationProtocol(libp2p::Host &host,
                                                        const blockchain::GenesisBlockHash &genesis,
                                                        std::shared_ptr<Beefy> beefy)
     : RequestResponseProtocolType{
           kName,
           host,
           make_protocols(kBeefyJustificationProtocol, genesis),
           log::createLogger(kName),
       },
       beefy_{std::move(beefy)} {}

  std::optional<outcome::result<BeefyJustificationProtocol::ResponseType>>
  BeefyJustificationProtocol::onRxRequest(RequestType block,
                                          std::shared_ptr<Stream>) {
    OUTCOME_TRY(opt, beefy_->getJustification(block));
    if (opt) {
      return outcome::success(std::move(*opt));
    }
    return outcome::failure(ProtocolError::NO_RESPONSE);
  }
}  // namespace kagome::network
