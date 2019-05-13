/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MUXER_HPP
#define KAGOME_STREAM_MUXER_HPP

#include <functional>
#include <memory>
#include <optional>

#include <outcome/outcome.hpp>
#include "libp2p/multi/multistream.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::muxer {
  /**
   * Stream muxer, allowing to create streams over the underlying connection
   */
  class StreamMuxer {
   public:
    /**
     * Start a StreamMuxer instance; it is going to read a connection and
     * accept new messages and streams
     */
    virtual void start() = 0;

    /**
     * Stop a StreamMuxer instance
     */
    virtual void stop() = 0;

    /**
     * Create a new stream over this muxer
     * @return pointer to a created stream in case of success, error otherwise
     */
    virtual outcome::result<std::unique_ptr<stream::Stream>> newStream() = 0;

    /**
     * Close an underlying connection
     */
    virtual void close() = 0;

    /**
     * Check, if an underlying connection is closed
     * @return true, if it is closed, false otherwise
     */
    virtual bool isClosed() = 0;

    virtual ~StreamMuxer() = default;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_STREAM_MUXER_HPP
