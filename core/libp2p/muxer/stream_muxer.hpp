/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MUXER_HPP
#define KAGOME_STREAM_MUXER_HPP

#include <memory>

#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/connection.hpp"
#include "libp2p/transport/muxed_connection.hpp"

namespace libp2p::muxer {
  /**
   * Strategy to upgrade connections to muxed
   */
  class StreamMuxer {
   public:
    using NewStreamHandler =
        std::function<void(std::unique_ptr<stream::Stream>)>;

//    /**
//     * Upgrade connection to the muxed one
//     * @param connection to be upgraded
//     * @param stream_handler - handler to be called, when new streams are
//     * received over this connection
//     * @return pointer to the upgraded connection
//     */
//    virtual std::unique_ptr<transport::MuxedConnection> upgrade(
//        std::shared_ptr<transport::Connection> connection,
//        NewStreamHandler stream_handler) const = 0;

    virtual ~StreamMuxer() = default;
  };
}  // namespace libp2p::muxer

#endif  //KAGOME_STREAM_MUXER_HPP
