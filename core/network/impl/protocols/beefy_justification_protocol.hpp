/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/fetch_justification.hpp"
#include "consensus/beefy/types.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/types/roles.hpp"

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::network {
  class Beefy;
  class PeerManager;
  struct StreamEngine;
}  // namespace kagome::network

namespace kagome::network {

  class BeefyJustificationProtocol
      : public consensus::beefy::FetchJustification,
        public RequestResponseProtocol<primitives::BlockNumber,
                                       consensus::beefy::BeefyJustification,
                                       ScaleMessageReadWriter> {
    static constexpr auto kName = "BeefyJustificationProtocol";

   public:
    BeefyJustificationProtocol(libp2p::Host &host,
                               const blockchain::GenesisBlockHash &genesis,
                               common::MainThreadPool &main_thread_pool,
                               std::shared_ptr<PeerManager> peer_manager,
                               std::shared_ptr<Beefy> beefy);

    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType block, std::shared_ptr<Stream>) override;

    void onTxRequest(const RequestType &) override {}

    void fetchJustification(primitives::BlockNumber) override;

   private:
    std::weak_ptr<BeefyJustificationProtocol> weak_from_this() {
      return std::static_pointer_cast<BeefyJustificationProtocol>(
          shared_from_this());
    }

    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<Beefy> beefy_;
    bool fetching_ = false;
  };

}  // namespace kagome::network
