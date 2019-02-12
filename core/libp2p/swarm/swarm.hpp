/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SWARM_HPP
#define KAGOME_SWARM_HPP

#include <rxcpp/rx-observable.hpp>
#include "common/result.hpp"
#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/common_objects/multihash.hpp"
#include "libp2p/common_objects/peer.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/swarm/dial_status.hpp"

namespace libp2p {
  namespace swarm {
    /**
     * Establishes connections with other peers
     */
    class Swarm {
     public:
      /**
       * Establish connection with the peer via the best possible transport
       * @param peer to connect to
       * @return observable to connection
       */
      virtual rxcpp::observable<
          kagome::expected::Result<DialStatus, std::string>>
      dial(const common::Peer::PeerInfo &peer) = 0;
    };
  }  // namespace swarm
}  // namespace libp2p

#endif  // KAGOME_SWARM_HPP
