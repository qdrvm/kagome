/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/types/roles.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {
  class Beefy;
  struct StreamEngine;
}  // namespace kagome::network

namespace kagome::network {

  class BeefyJustificationProtocol
      : public RequestResponseProtocol<primitives::BlockNumber,
                                       consensus::beefy::BeefyJustification,
                                       ScaleMessageReadWriter> {
    static constexpr auto kName = "BeefyJustificationProtocol";

   public:
    BeefyJustificationProtocol(libp2p::Host &host,
                               const blockchain::GenesisBlockHash &genesis,
                               std::shared_ptr<Beefy> beefy);

    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType block, std::shared_ptr<Stream>) override;

    void onTxRequest(const RequestType &) override {}

   private:
    std::shared_ptr<Beefy> beefy_;
  };

}  // namespace kagome::network
