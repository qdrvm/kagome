/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MANAGER_HPP
#define KAGOME_STREAM_MANAGER_HPP

#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::network {

  // manager for application-level protocols
  // in go implementation called Switch
  // https://sourcegraph.com/github.com/libp2p/go-libp2p-core@consolidate-skeleton/-/blob/host/host.go#L37
  struct StreamManager {
    using StreamHandler = void(std::shared_ptr<stream::Stream>);

    virtual ~StreamManager() = default;

    /**
     * @brief Set handler for the {@param protocol}
     * @param protocol protocol
     * @param handler function-handler
     */
    virtual void setProtocolHandler(
        const peer::Protocol &protocol,
        const std::function<StreamHandler> &handler) = 0;

    /**
     * @brief Set handler for the protocol. First, searches all handlers by
     * given prefix, then executes handler callback for all matches when {@param
     * predicate} returns true.
     */
    virtual void setProtocolHandlerByPrefix(
        std::string_view prefix,
        const std::function<bool(const peer::Protocol &)> &predicate,
        const std::function<StreamHandler> &handler) = 0;

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
    virtual void handle(const peer::Protocol &p,
                        std::shared_ptr<stream::Stream> stream) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_ROUTER_HPP
