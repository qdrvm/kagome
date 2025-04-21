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
#include "common/main_thread_pool.hpp"
#include "log/logger.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/peer_manager.hpp"
#include "network/protocols/req_pov_protocol.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {

  struct ReqPovProtocolInner;
  class ReqPovProtocolImpl final : public ReqPovProtocol,
                                   NonCopyable,
                                   NonMovable {
   public:
    ReqPovProtocolImpl() = delete;
    ~ReqPovProtocolImpl() override = default;

    ReqPovProtocolImpl(RequestResponseInject inject,
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
    std::shared_ptr<ReqPovProtocolInner> impl_;
  };

}  // namespace kagome::network
