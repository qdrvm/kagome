/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include <outcome/outcome.hpp>
#include "libp2p/common/network_message.hpp"

namespace libp2p::stream {
  /**
   * Stream between two peers in the network
   */
  class Stream {
   public:
    /**
     * Read one frame - unit of data exchange in streams - from this stream
     * @return result with frame
     */
    virtual outcome::result<common::NetworkMessage> readFrame() = 0;

    /**
     * Write data to the stream
     * @param msg to be written
     * @return void in case of success, error otherwise
     */
    virtual outcome::result<void> writeFrame(
        const common::NetworkMessage &msg) = 0;

    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const = 0;

    /**
     * Check, if this stream is closed from this side of the connection and thus
     * cannot be written to
     * @return true, of stream cannot be written to, falst otherwise
     */
    virtual bool isClosedForWrite() const = 0;

    /**
     * Close this stream from this side, so that no writes can be done
     */
    virtual void close() = 0;

    /**
     * Break the connection entirely, so that no writes or reads can be done
     */
    virtual void reset() = 0;

    virtual ~Stream() = 0;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_STREAM_HPP
