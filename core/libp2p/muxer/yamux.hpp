/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_HPP
#define KAGOME_LIBP2P_YAMUX_HPP

#include <map>
#include <vector>

#include <outcome/outcome.hpp>
#include "libp2p/muxer/muxer.hpp"
#include "libp2p/muxer/yamux_config.hpp"
#include "libp2p/stream/yamux_stream.hpp"
#include "libp2p/transport/connection.hpp"

namespace libp2p::muxer {
  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class Yamux : public Muxer {
    friend stream::YamuxStream;

    /// according to spec, there is a 32-bit number for stream id
    using StreamId = uint32_t;

   public:
    /**
     * Create a Yamux multiplexer
     * @param connection over which it should be created
     * @param config - parameters of this instance
     */
    Yamux(transport::Connection &connection, YamuxConfig config);

    std::unique_ptr<stream::Stream> newStream() override;

   private:
    enum class ReadWriteError { kConnectionError = 1, kStreamError };

    outcome::result<void> write(StreamId stream_id,
                                const common::NetworkMessage &msg) const;
    outcome::result<common::NetworkMessage> read(StreamId stream_id) const;

    bool is_server_;
    transport::Connection &connection_;
    StreamId last_created_stream_id;

    /// as streams are full-duplex, there's a possibility we close the stream
    /// from out side and thus cannot write to it, but still can receive data
    std::vector<StreamId> writable_streams_;

    /// buffers, containing messages, which came for streams, but were not yet
    /// read; also used to see, if the stream is still active
    std::map<StreamId, std::vector<common::NetworkMessage>> stream_buffers_;

    /**
     * Get a stream id for a new stream
     * @return stream id
     */
    StreamId getNewStreamId();

    /**
     * Half-close stream with a given id - means we will not send messages to
     * it, but they still can be received
     * @param stream_id of the stream to be closed
     */
    void closeStream(StreamId stream_id);

    // stream->write(msg) { yamux_->write(id_, msg); }
    // yamux->write(streamId, msg) { id = map[stream]; conn_->write(msg); }

    // yamux->read(id) { await return buffers[id]; }

    // close(StreamId) -> IMPLEMENT
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_LIBP2P_YAMUX_HPP
