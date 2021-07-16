/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include <libp2p/connection/stream.hpp>

#include "network/protocols/block_announce_protocol.hpp"
#include "network/protocols/grandpa_protocol.hpp"
#include "network/protocols/propagate_transactions_protocol.hpp"
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
    virtual std::shared_ptr<PropagateTransactionsProtocol>
    getPropagateTransactionsProtocol() const = 0;
    virtual std::shared_ptr<SyncProtocol> getSyncProtocol() const = 0;
    virtual std::shared_ptr<GrandpaProtocol> getGrandpaProtocol() const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_ROUTER_HPP
