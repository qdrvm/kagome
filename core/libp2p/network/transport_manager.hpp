/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_MANAGER_HPP
#define KAGOME_TRANSPORT_MANAGER_HPP

#include <memory>

#include <gsl/span>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p::network {

  /**
   * @brief Transport Manager knows about all available transport instances and
   * allows to query them
   */
  struct TransportManager {
   protected:
    using TransportSPtr = std::shared_ptr<transport::Transport>;

   public:
    virtual ~TransportManager() = default;

    /**
     * Add a transport to this transport manager
     * @param t - transport to be added
     */
    virtual void add(TransportSPtr t) = 0;

    /**
     * Add transports to this transport manager
     * @param t - transports to be added
     */
    virtual void add(gsl::span<const TransportSPtr> t) = 0;

    /**
     * Get all transports, supported by this manager
     * @return transports
     */
    virtual gsl::span<const TransportSPtr> getAll() const = 0;

    /**
     * Remove all transports from the manager
     */
    virtual void clear() = 0;

    /**
     * @brief Finds best transport for the given multiaddress.
     *
     * "Best" transport is a transport that:
     * 1. returns true on `canDial(multiaddr)`;
     *  @and
     * 2. is chosen via a given strategy; by default, the strategy is "first
     * transport available"
     * TODO(warchant) 29.05.19 [PRE-195]: add strategies
     *
     * @param ma - multiaddress, which we are going to connect to
     * @return nullptr if no transport available, pointer to transport instance
     * otherwise
     */
    virtual TransportSPtr findBest(const multi::Multiaddress &ma) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_TRANSPORT_MANAGER_HPP
