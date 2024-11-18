/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/peer_manager.hpp"
#include "network/protocols/req_pov_protocol.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {

  struct ReqPovProtocolImpl;

  class ReqPovProtocol final : public IReqPovProtocol, NonCopyable, NonMovable {
   public:
    ReqPovProtocol() = delete;
    ~ReqPovProtocol() override = default;

    ReqPovProtocol(libp2p::Host &host,
                   const application::ChainSpec &chain_spec,
                   const blockchain::GenesisBlockHash &genesis_hash,
                   std::shared_ptr<ReqPovObserver> observer);

    const Protocol &protocolName() const override;

    bool start() override;

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerId &peer_id,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void request(const PeerId &peer_id,
                 RequestPov,
                 std::function<void(outcome::result<ResponsePov>)>
                     &&response_handler) override;

   private:
    std::shared_ptr<ReqPovProtocolImpl> impl_;
  };

}  // namespace kagome::network
