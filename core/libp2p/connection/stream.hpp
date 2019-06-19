/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STREAM_HPP
#define KAGOME_CONNECTION_STREAM_HPP

#include <outcome/outcome.hpp>
#include "libp2p/basic/readwritecloser.hpp"

namespace libp2p::connection {

  /**
   * Stream over some connection, allowing to write/read to/from that connection
   * @note the user MUST WAIT for the completion of method from this list
   * before calling another method from this list:
   *    - write
   *    - writeSome
   *    - close
   *    - adjustWindowSize
   *    - reset
   * Also, 'read' & 'readSome' are in another tuple. This behaviour results in
   * possibility to read and write simultaneously, but double read or write is
   * forbidden
   */
  struct Stream : public basic::ReadWriteCloser {
    ~Stream() override = default;

    /**
     * Check, if this stream is closed from this side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const = 0;

    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be written to
     * @return true, if stream cannot be written to, false otherwise
     */
    virtual bool isClosedForWrite() const = 0;

    /**
     * @brief Close this stream entirely; this normally means an error happened,
     * so it should not be used just to close the stream
     * @param cb to be called, when the operation succeeds of fails
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void reset(std::function<void(outcome::result<void>)> cb) = 0;

    /**
     * Set a new receive window size of this stream - how much unacknowledged
     * (not read) bytes can we on our side of the stream
     * @param new_size for the window
     * @param cb to be called, when the operation succeeds of fails
     * @note the method MUST NOT be called until the last 'write' or 'writeSome'
     * or 'close' or 'adjustWindowSize' or 'reset' completes
     */
    virtual void adjustWindowSize(
        uint32_t new_size, std::function<void(outcome::result<void>)> cb) = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_STREAM_HPP
