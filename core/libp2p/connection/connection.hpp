/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_CONNECTION_HPP
#define UGUISU_CONNECTION_HPP

#include <vector>

#include "common/result.hpp"
#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/common_objects/network_message.hpp"
#include "libp2p/common_objects/peer.hpp"

namespace libp2p {
  namespace connection {
    /**
     * Point-to-point link to the other peer
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
      virtual common::Peer::PeerInfo getPeerInfo() const = 0;

      /**
       * Set information about the peer this connection connects to
       * @param info to be set
       */
      virtual void setPeerInfo(const common::Peer::PeerInfo &info) = 0;

      /**
       * Write message to the connection
       * @param msg to be written
       * @return void in case of success, error otherwise
       */
      virtual uguisu::expected::Result<void, std::string> write(
          const common::NetworkMessage &msg) const = 0;

      /**
       * Read message from the connection
       * @return message, received by that connection, in case of success, error
       * otherwise
       */
      virtual uguisu::expected::Result<common::NetworkMessage, std::string>
      read() const = 0;
    };
  }  // namespace connection
}  // namespace libp2p

#endif  // UGUISU_CONNECTION_HPP
