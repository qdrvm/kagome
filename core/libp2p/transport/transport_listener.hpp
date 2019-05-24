/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_LISTENER_HPP
#define KAGOME_TRANSPORT_LISTENER_HPP

#include <boost/signals2/connection.hpp>
#include <functional>
#include <memory>
#include <vector>

#include "libp2p/basic/closeable.hpp"
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport {

  /**
   * Listens to the connections from the specified addresses and reacts when
   * receiving ones
   */
  class TransportListener : public basic::Closeable {
   public:
    using NoArgsCallback = void();
    using ErrorCallback = void(const std::error_code &);
    using MultiaddrCallback = void(const multi::Multiaddress &);
    using ConnectionCallback =
        outcome::result<void>(std::shared_ptr<connection::RawConnection>);
    using HandlerFunc = std::function<ConnectionCallback>;
    using ErrorFunc = std::function<ErrorCallback>;

    ~TransportListener() override = default;

    /**
     * Switch the listener into 'listen' mode; it will react to every new
     * connection
     * @param address to listen to
     */
    virtual outcome::result<void> listen(
        const multi::Multiaddress &address) = 0;

    /**
     * @brief Returns true if this transport can listen on given multiaddress,
     * false otherwise.
     * @param address multiaddress
     */
    virtual bool canListen(const multi::Multiaddress &address) const = 0;

    /**
     * Get addresses, which this listener listens to
     * @return collection of those addresses
     */
    virtual outcome::result<multi::Multiaddress> getListenMultiaddr() const = 0;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_LISTENER_HPP
