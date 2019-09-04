/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MANAGER_HPP
#define KAGOME_STREAM_MANAGER_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>

namespace kagome::network {
  /**
   * Manages streams, allowing the client to reuse them
   * @tparam PeerIdentifier - unique identifier of the peer the stream is opened
   * to
   * @tparam ProtocolT - protocol, over which the stream is opened
   * @tparam Stream - type of the stream
   */
  template <typename PeerIdentifier, typename ProtocolT, typename Stream>
  struct StreamManager {
    virtual ~StreamManager() = default;

    /**
     * Submit a new stream to the manager; if stream to that peer already
     * exists, it will be reset and substituted by a new one
     * @param id of the peer
     * @param protocol, over which the stream is opened
     * @param stream to be submitted
     */
    virtual void submitStream(const PeerIdentifier &id,
                              const ProtocolT &protocol,
                              std::shared_ptr<Stream> stream) = 0;

    /**
     * Get a stream to that peer; if no such stream exists, it will be opened
     * @param id of the peer
     * @param protocol, over which the stream should be opened
     * @param cb with pointer to the stream or error
     */
    virtual void getStream(
        const PeerIdentifier &id,
        const ProtocolT &protocol,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> cb) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_STREAM_MANAGER_HPP
