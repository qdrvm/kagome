/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/protocol/ping.hpp>

#include "network/impl/protocols/block_announce_protocol.hpp"
#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/impl/protocols/parachain_protocols.hpp"
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/protocol_req_pov.hpp"
#include "network/protocols/state_protocol.hpp"
#include "network/protocols/sync_protocol.hpp"

namespace kagome::network {
  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   */
  class Router {
   public:
    virtual ~Router() = default;

    virtual std::shared_ptr<BlockAnnounceProtocol> getBlockAnnounceProtocol()
        const = 0;
    virtual std::shared_ptr<CollationProtocol> getCollationProtocol() const = 0;
    virtual std::shared_ptr<ValidationProtocol> getValidationProtocol()
        const = 0;
    virtual std::shared_ptr<ReqCollationProtocol> getReqCollationProtocol()
        const = 0;
    virtual std::shared_ptr<ReqPovProtocol> getReqPovProtocol() const = 0;
    virtual std::shared_ptr<FetchChunkProtocol> getFetchChunkProtocol()
        const = 0;
    virtual std::shared_ptr<FetchAvailableDataProtocol>
    getFetchAvailableDataProtocol() const = 0;
    virtual std::shared_ptr<StatmentFetchingProtocol>
    getFetchStatementProtocol() const = 0;
    virtual std::shared_ptr<PropagateTransactionsProtocol>
    getPropagateTransactionsProtocol() const = 0;
    virtual std::shared_ptr<StateProtocol> getStateProtocol() const = 0;
    virtual std::shared_ptr<SyncProtocol> getSyncProtocol() const = 0;
    virtual std::shared_ptr<GrandpaProtocol> getGrandpaProtocol() const = 0;

    virtual std::shared_ptr<libp2p::protocol::Ping> getPingProtocol() const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_ROUTER_HPP
