/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_HPP
#define KAGOME_CONNECTION_HPP

#include <vector>

#include <boost/optional.hpp>
#include "common/result.hpp"
#include "libp2p/basic_interfaces/writable.hpp"
#include "libp2p/common_objects/network_message.hpp"
#include "libp2p/common_objects/peer_info.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p {
  namespace connection {
    /**
     * Point-to-point link to the other peer
     */
    class Connection : public basic_interfaces::Writable {
      /**
       * Get addresses, which are observed with an underlying transport
       * @return collection of such addresses
       */
      virtual std::vector<multi::Multiaddress> getObservedAdrresses() const = 0;

      /**
       * Get information about the peer this connection connects to
       * @return peer information if set, none otherwise
       */
      virtual boost::optional<common::PeerInfo> getPeerInfo() const = 0;

      /**
       * Set information about the peer this connection connects to
       * @param info to be set
       */
      virtual void setPeerInfo(const common::PeerInfo &info) = 0;

      /**
       * Read message from the connection
       * @return optional to message, received by that connection
       */
      virtual boost::optional<common::NetworkMessage> read() const = 0;
    };
  }  // namespace connection
}  // namespace libp2p

#endif  // KAGOME_CONNECTION_HPP
