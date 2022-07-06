/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_COLLATIONPROTOCOL
#define KAGOME_NETWORK_COLLATIONPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/collation_observer.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/status.hpp"

namespace kagome::network {

  class CollationProtocol final : public ProtocolBaseImpl<CollationProtocol> {
   public:
    CollationProtocol() = delete;
    ~CollationProtocol() override = default;

    CollationProtocol(libp2p::Host &host,
                      application::ChainSpec const &chain_spec,
                      std::shared_ptr<CollationObserver> observer);

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

   private:
    libp2p::Host &host_;
    std::shared_ptr<CollationObserver> observer_;
    Protocol const protocol_;
    log::Logger log_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_COLLATIONPROTOCOL
