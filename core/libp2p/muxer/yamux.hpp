/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_HPP
#define KAGOME_LIBP2P_YAMUX_HPP

#include "libp2p/connection/muxed_connection.hpp"

namespace libp2p::muxer {
  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class Yamux : public connection::MuxedConnection {
   public:
    Yamux(conn, isServer);

    std::unique_ptr<stream::Stream> newStream() override;

   private:
    // map<Stream, Id>

    // buffers<Id, vector<NetworkMsg>>

    // stream->write(msg) { yamux_->write(id_, msg); }
    // yamux->write(streamId, msg) { id = map[stream]; conn_->write(msg); }

    // yamux->read(...) { await return buffers[id]; }
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_LIBP2P_YAMUX_HPP
