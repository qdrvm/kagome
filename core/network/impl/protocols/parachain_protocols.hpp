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
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/collation_observer.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/parachain_protocol.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/roles.hpp"
#include "network/validation_observer.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  class CollationProtocol : public ParachainProtocol<CollationObserver,
                                                     CollationProtocolMessage,
                                                     true, network::CollationVersion::V1> {
   public:
    CollationProtocol(libp2p::Host &host,
                      Roles roles,
                      const application::ChainSpec &chain_spec,
                      const blockchain::GenesisBlockHash &genesis_hash,
                      std::shared_ptr<ObserverType> observer,
                      std::shared_ptr<network::PeerView> peer_view)
        : ParachainProtocol(
            host,
            roles,
            chain_spec,
            genesis_hash,
            std::move(observer),
            kCollationProtocol,
            std::move(peer_view),
            log::createLogger("CollationProtocol", "collation_protocol")){};
  };

  class CollationProtocolVStaging
      : public ParachainProtocol<CollationObserver,
                                 vstaging::CollatorProtocolMessage,
                                 true, network::CollationVersion::VStaging> {
   public:
    CollationProtocolVStaging(libp2p::Host &host,
                              Roles roles,
                              const application::ChainSpec &chain_spec,
                              const blockchain::GenesisBlockHash &genesis_hash,
                              std::shared_ptr<ObserverType> observer,
                              std::shared_ptr<network::PeerView> peer_view)
        : ParachainProtocol(
            host,
            roles,
            chain_spec,
            genesis_hash,
            std::move(observer),
            kCollationProtocolVStaging,
            std::move(peer_view),
            log::createLogger("CollationProtocolVStaging", "collation_protocol_vstaging")){};
  };

  class ValidationProtocol : public ParachainProtocol<ValidationObserver,
                                                      ValidatorProtocolMessage,
                                                      false, network::CollationVersion::V1> {
   public:
    ValidationProtocol(libp2p::Host &host,
                       Roles roles,
                       const application::ChainSpec &chain_spec,
                       const blockchain::GenesisBlockHash &genesis_hash,
                       std::shared_ptr<ObserverType> observer,
                       std::shared_ptr<network::PeerView> peer_view)
        : ParachainProtocol(
            host,
            roles,
            chain_spec,
            genesis_hash,
            std::move(observer),
            kValidationProtocol,
            std::move(peer_view),
            log::createLogger("ValidationProtocol", "validation_protocol")){};
  };

  class ValidationProtocolVStaging
      : public ParachainProtocol<ValidationObserver,
                                 vstaging::ValidatorProtocolMessage,
                                 false, network::CollationVersion::VStaging> {
   public:
    ValidationProtocolVStaging(libp2p::Host &host,
                       Roles roles,
                       const application::ChainSpec &chain_spec,
                       const blockchain::GenesisBlockHash &genesis_hash,
                       std::shared_ptr<ObserverType> observer,
                       std::shared_ptr<network::PeerView> peer_view)
        : ParachainProtocol(
            host,
            roles,
            chain_spec,
            genesis_hash,
            std::move(observer),
            kValidationProtocolVStaging,
            std::move(peer_view),
            log::createLogger("ValidationProtocolVStaging", "validation_protocol_vstaging")){};
  };

}  // namespace kagome::network
