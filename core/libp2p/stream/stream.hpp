/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include "libp2p/transport/connection.hpp"

namespace libp2p::stream {
  /**
   * Stream between two peers in the network
   */
  class Stream : public transport::Connection {
    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const = 0;

    /**
     * Check, if this stream is closed from this side of the connection and thus
     * cannot be written to
     * @see Closeable::isClosed
     */
    bool isClosed() const override = 0;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_STREAM_HPP
