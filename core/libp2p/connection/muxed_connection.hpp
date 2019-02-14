/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXED_CONNECTION_HPP
#define KAGOME_MUXED_CONNECTION_HPP

#include "libp2p/connection/connection.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::connection {
  /**
   * Connection complement, which appears, when it gets muxed
   */
  class MuxedConnection : public Connection {
    /**
     * Create a new stream over this muxed connection
     * @return pointer to a created stream
     */
    virtual std::unique_ptr<stream::Stream> newStream() = 0;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_MUXED_CONNECTION_HPP
