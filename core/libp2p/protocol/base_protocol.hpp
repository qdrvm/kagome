/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_PROTOCOL_HPP
#define KAGOME_BASE_PROTOCOL_HPP

#include "libp2p/connection/stream.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::protocol {

  /**
   * @brief Base class for all application-level protocols based on streams.
   */
  struct BaseProtocol {
    virtual ~BaseProtocol() = default;

    using StreamSPtr = std::shared_ptr<connection::Stream>;
    using StreamSPtrFunc = std::function<void(StreamSPtr)>;

    using StreamSPtrResult = outcome::result<StreamSPtr>;
    using StreamSPtrResultFunc = std::function<void(StreamSPtrResult)>;

    /**
     * @brief Getter for a unique identifier for this protocol.
     * @example /yamux/1.0.0 or /ping/1.0.0
     * @return protocol identifier
     */
    virtual peer::Protocol getProtocolId() const noexcept = 0;

    /**
     * @brief Server-side handler, invoked when client is opened stream to us.
     * @param cb callback executed when new stream opened to us
     */
    virtual void onStream(StreamSPtrFunc cb) = 0;

    /**
     * @brief Client-side handler, invoked when we (client) successfully
     * established connection to peer p.
     * @param p remote peer
     * @param cb callback executed when we successfully opened stream to remote
     * peer
     */
    virtual void newStream(const peer::PeerInfo &p,
                           StreamSPtrResultFunc cb) = 0;
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_BASE_PROTOCOL_HPP
