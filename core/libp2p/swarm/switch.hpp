/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SWITCH_HPP
#define KAGOME_SWITCH_HPP

#include <outcome/outcome.hpp>

#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/swarm/protocol_manager.hpp"

namespace libp2p::swarm {

  struct Switch {
    virtual ~Switch() = default;

    /**
     * @brief Initiates connection to the peer {@param p}. If connection exists,
     * does nothing, otherwise blocks until successful connection is created or
     * error happens.
     * @param p peer to connect. Addresses will be searched in PeerRepository.
     * If not found, will be searched using Routing module.
     * @return nothing on success, error otherwise.
     */
    virtual outcome::result<void> connect(const peer::PeerId &p) = 0;

    /**
     * @brief Open new stream to the peer {@param p} with protocol {@param
     * protocol}.
     * @param p stream will be opened with this peer
     * @param protocol "speak" using this protocol
     * @param handler callback, will be executed on successful stream creation
     * @return May return error if peer {@param p} is not known to this peer.
     */

    // TODO(@warchant):
    // 1. if upgraded connection to p exists, then re-use it and do
    // muxer.newStream()
    // 2. if no upgraded connection to p exist, and we know multiaddress of p
    // (via peer repository), then connect + upgrade, then muxer.newStream()
    // 3. if no peer p in peer repository, return error "unknown peer
    // address"
    virtual outcome::result<void> newStream(
        const peer::PeerInfo &p, const peer::Protocol &protocol,
        const std::function<stream::Stream::Handler> &handler) = 0;
  };

}  // namespace libp2p::swarm

#endif  // KAGOME_SWITCH_HPP
