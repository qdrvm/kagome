/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace libp2p::stream {
  /**
   * Stream between two peers in the network
   */
  class Stream {
   public:
    using NetworkMessageOutcome = outcome::result<kagome::common::Buffer>;
    using ReadCompletionHandler = std::function<void(NetworkMessageOutcome)>;

    using ErrorCodeCallback = std::function<void(std::error_code, size_t)>;

    /**
     * Read one frame - unit of data exchange in streams - from this stream
     * @param completion_handler - function, which is going to be called, when
     * message is read from the stream
     */
    virtual void readAsync(ReadCompletionHandler completion_handler) = 0;

    /**
     * Write data to the stream
     * @param msg to be written
     */
    virtual void writeAsync(const kagome::common::Buffer &msg) = 0;

    /**
     * Write data to the stream
     * @param msg to be written
     * @param error_callback - callback, which is going to be called, when the
     * write finishes
     */
    virtual void writeAsync(const kagome::common::Buffer &msg,
                            ErrorCodeCallback error_callback) = 0;

    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const = 0;

    /**
     * Check, if this stream is closed from this side of the connection and thus
     * cannot be written to
     * @return true, of stream cannot be written to, false otherwise
     */
    virtual bool isClosedForWrite() const = 0;

    /**
     * Check, if this stream is closed entirely
     * @return true, if the stream is closed, false otherwise
     */
    virtual bool isClosedEntirely() const = 0;

    /**
     * Close this stream from this side, so that no writes can be done
     */
    virtual void close() = 0;

    /**
     * Break the connection entirely, so that no writes or reads can be done
     */
    virtual void reset() = 0;

    virtual ~Stream() = default;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_STREAM_HPP
