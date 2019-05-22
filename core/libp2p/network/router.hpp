/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include "libp2p/connection/stream.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::network {

  // manager for application-level protocols
  // in go implementation called Switch
  // https://sourcegraph.com/github.com/libp2p/go-libp2p-core@consolidate-skeleton/-/blob/host/host.go#L37
  struct Router {
    virtual ~Router() = default;

    /**
     * @brief Set handler for the {@param protocol}
     * @param protocol protocol
     * @param handler function-handler
     */
    virtual void setProtocolHandler(
        const peer::Protocol &protocol,
        const std::function<connection::Stream::Handler> &handler) = 0;

    /**
     * @brief Set handler for the protocol. First, searches all handlers by
     * given prefix, then executes handler callback for all matches when {@param
     * predicate} returns true.
     */
    virtual void setProtocolHandlerByPrefix(
        std::string_view prefix,
        const std::function<bool(const peer::Protocol &)> &predicate,
        const std::function<connection::Stream::Handler> &handler) = 0;

    /**
     * @brief Returns list of supported protocols.
     */
    virtual std::vector<peer::Protocol> getSupportedProtocols() const = 0;

    /**
     * @brief Removes all associated handlers to given protocol.
     * @param protocol
     */
    virtual void removeProtocolHandler(const peer::Protocol &protocol) = 0;

    /**
     * @brief Remove all handlers.
     */
    virtual void removeAll() = 0;

   protected:
    /**
     * @brief Execute stored handler for given protocol {@param p}
     * @param p handler for this protocol will be invoked
     * @param stream with this stream
     */
    virtual outcome::result<void> handle(
        const peer::Protocol &p,
        std::shared_ptr<connection::Stream> stream) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_ROUTER_HPP
