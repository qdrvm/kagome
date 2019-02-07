/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_CONNECTION_HPP
#define UGUISU_CONNECTION_HPP

#include <vector>

#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/common_objects/peer.hpp"

namespace libp2p {
  namespace connection {
    /**
     * Offers a possibility to read/write data to the client(s) connection(s)
     */
    class Connection {
      /**
       * Get addresses, which are observed with an underlying transport
       * @return collection of such addresses
       */
      virtual std::vector<common::Multiaddress> getObservedAdrresses()
          const = 0;

      /**
       * Get information about the peer this connection connects to
       * @return peer information
       */
      virtual common::Peer::PeerInfo gerPeerInfo() const = 0;

      /**
       * Set information about the peer this connection connects to
       * @param info to be set
       */
      virtual void setPeerInfo(const common::Peer::PeerInfo &info) = 0;
    };
  }  // namespace connection
}  // namespace libp2p

#endif  // UGUISU_CONNECTION_HPP
