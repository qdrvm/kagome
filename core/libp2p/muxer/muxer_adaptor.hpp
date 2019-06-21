/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_ADAPTOR_HPP
#define KAGOME_MUXER_ADAPTOR_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/muxer/muxed_connection_config.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::connection {
  struct Stream;
}

namespace libp2p::muxer {
  /**
   * Strategy to upgrade connections to muxed
   */
  struct MuxerAdaptor {
    using StreamHandler =
        void(outcome::result<std::shared_ptr<connection::Stream>>);
    using StreamHandlerFunc = std::function<StreamHandler>;

    virtual ~MuxerAdaptor() = default;

    /**
     * Get a string identifier, associated with this adaptor
     * @return protocol id of the adaptor
     * @example '/yamux/1.0.0'
     */
    virtual peer::Protocol getProtocolId() const = 0;

    /**
     * Make a muxed connection from the secure one, using this adaptor
     * @param conn - connection to be upgraded
     * @param handler - function, which is called, when new streams arrive over
     * this connection
     * @param config of muxer to be created over the connection
     * @return upgraded connection or error
     */
    virtual outcome::result<std::shared_ptr<connection::CapableConnection>>
    muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                  StreamHandlerFunc handler,
                  MuxedConnectionConfig config) const = 0;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXER_ADAPTOR_HPP
