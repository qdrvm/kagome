/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_HPP
#define KAGOME_CONNECTION_HPP

#include <vector>

#include <optional>

#include "common/result.hpp"
#include "libp2p/basic/closeable.hpp"
#include "libp2p/basic/readable.hpp"
#include "libp2p/basic/writable.hpp"
#include "libp2p/common/network_message.hpp"
#include "libp2p/common/peer_info.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport {
  /**
   * Point-to-point link to the other peer
   */
  class Connection : public basic::Readable,
                     public basic::Writable,
                     public basic::Closeable {
   public:
    virtual ~Connection() = default;

    /**
     * This method retrieves the observed addresses we get from the underlying
     * transport, if any.
     * @return collection of such addresses
     */
    virtual outcome::result<std::vector<multi::Multiaddress>>
    getObservedAddresses() const = 0;

    /**
     * Get information about the peer this connection connects to
     * @return peer information if set, none otherwise
     */
    virtual std::optional<common::PeerInfo> getPeerInfo() const = 0;

    /**
     * Set information about the peer this connection connects to
     * @param info to be set
     */
    virtual void setPeerInfo(const common::PeerInfo &info) = 0;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_CONNECTION_HPP
