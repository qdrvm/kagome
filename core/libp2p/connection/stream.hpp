/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STREAM_HPP
#define KAGOME_CONNECTION_STREAM_HPP

#include <vector>

#include <gsl/span>
#include <outcome/outcome.hpp>

namespace libp2p::connection {

  /**
   * Full-duplex stream over some connection
   */
  struct Stream {
    using Handler = void(std::shared_ptr<Stream>);
    using ReadCallback = void(outcome::result<std::vector<uint8_t>>);
    using WriteCallback = void(outcome::result<size_t>);

    virtual ~Stream() = default;

    /**
     * @brief Read all (\param bytes) from the stream
     * @param bytes - number of bytes to read
     * @param cb - callback, which is called, when all bytes are read or error
     * happens
     * @note the method MUST NOT be called until the last 'read' or 'readSome'
     * completes
     */
    virtual void read(size_t bytes, std::function<ReadCallback> cb) = 0;

    /**
     * @brief Read any number of (but not more than) (\param bytes) from the
     * stream
     * @param bytes - number of bytes to read
     * @param cb - callback, which is called, when any bytes are read or error
     * happens
     * @note the method MUST NOT be called until the last 'read' or 'readSome'
     * completes
     */
    virtual void readSome(size_t bytes, std::function<ReadCallback> cb) = 0;

    /**
     * @brief Write all data from {@param in} to the stream
     * @param in - bytes to be written
     * @param cb - callback, which is called, when all bytes are written or
     * error happens
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void write(gsl::span<const uint8_t> in,
                       std::function<WriteCallback> cb) = 0;

    /**
     * @brief Write any data from {@param in} to the stream
     * @param in - bytes to be written
     * @param cb - callback, which is called, when any bytes are written or
     * error happens
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void writeSome(gsl::span<const uint8_t> in,
                           std::function<WriteCallback> cb) = 0;

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
     * @brief Check, if this stream is closed for both writes and reads
     * @return true if closed, false otherwise
     */
    virtual bool isClosed() const = 0;

    /**
     * @brief Close this stream for writes
     * @param cb - callback, which is called when the operation finishes,
     * successfully or not
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void close(std::function<void(outcome::result<void>)> cb) = 0;

    /**
     * @brief Close this stream entirely; this normally means an error happened,
     * so it should not be used just to close the stream
     * @param cb - callback, which is called when the operation finishes,
     * successfully or not
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void reset(std::function<void(outcome::result<void>)> cb) = 0;

    /**
     * Set a new receive window size of this stream - how much unacknowledged
     * (not read) bytes can we on our side of the stream
     * @param new_size for the window
     * @param cb - callback, which is called when the operation finishes,
     * successfully or not
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void adjustWindowSize(
        uint32_t new_size, std::function<void(outcome::result<void>)> cb) = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_STREAM_HPP
