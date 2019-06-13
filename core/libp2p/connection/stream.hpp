/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STREAM_HPP
#define KAGOME_CONNECTION_STREAM_HPP

#include <continuable/continuable.hpp>
#include <gsl/span>
#include <outcome/outcome.hpp>

namespace libp2p::connection {

  /**
   * Full-duplex stream over some connection
   */
  struct Stream {
    using Handler = void(std::shared_ptr<Stream>);

    using ReadResult = outcome::result<std::vector<uint8_t>>;
    using WriteResult = outcome::result<size_t>;
    using VoidResult = outcome::result<void>;

    virtual ~Stream() = default;

    /**
     * @brief Read all (\param bytes) from the stream
     * @param bytes - number of bytes to read
     * @return continuable to the result of the read
     * @note the method MUST NOT be called until the last 'read' or 'readSome'
     * completes
     */
    virtual cti::continuable<ReadResult> read(size_t bytes) = 0;

    /**
     * @brief Read any number of (but not more than) (\param bytes) from the
     * stream
     * @param bytes - number of bytes to read
     * @return continuable to the result of the read
     * @note the method MUST NOT be called until the last 'read' or 'readSome'
     * completes
     */
    virtual cti::continuable<ReadResult> readSome(size_t bytes) = 0;

    /**
     * @brief Write all data from {@param in} to the stream
     * @param in - bytes to be written
     * @return continuable to the result of the write
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual cti::continuable<WriteResult> write(
        gsl::span<const uint8_t> in) = 0;

    /**
     * @brief Write any data from {@param in} to the stream
     * @param in - bytes to be written
     * @return continuable to the result of the write
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual cti::continuable<WriteResult> writeSome(
        gsl::span<const uint8_t> in) = 0;

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
     * @return continuable to the result of the close
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual cti::continuable<VoidResult> close() = 0;

    /**
     * @brief Close this stream entirely; this normally means an error happened,
     * so it should not be used just to close the stream
     * @return continuable to the result of the reset
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual cti::continuable<VoidResult> reset() = 0;

    /**
     * Set a new receive window size of this stream - how much unacknowledged
     * (not read) bytes can we on our side of the stream
     * @param new_size for the window
     * @return continuable to the result of the adjusting of the size
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual cti::continuable<VoidResult> adjustWindowSize(
        uint32_t new_size) = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_STREAM_HPP
