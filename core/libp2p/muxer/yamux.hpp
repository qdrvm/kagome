/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_HPP
#define KAGOME_LIBP2P_YAMUX_HPP

#include <map>
#include <set>

#include <outcome/outcome.hpp>
#include "libp2p/muxer/muxer.hpp"
#include "libp2p/muxer/yamux_config.hpp"

namespace libp2p::stream {
  class Stream;
  class YamuxStream;
}  // namespace libp2p::stream

namespace libp2p::muxer {

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class Yamux : public Muxer {
    friend class libp2p::stream::YamuxStream;

    /// according to spec, there is a 32-bit number for stream id
    using StreamId = uint32_t;

   public:
    /**
     * Create a Yamux multiplexer
     * @param connection over which it should be created
     * @param config - parameters of this instance
     */
    Yamux(transport::Connection &connection, YamuxConfig config);

    /**
     * Spawn a new stream over the underlying connection
     * @return pointer to new stream
     */
    std::unique_ptr<stream::Stream> newStream() override;

   private:
    /// YamuxStream API begins

    enum class ReadWriteError { kConnectionError = 1, kStreamError };

    outcome::result<kagome::common::Buffer> read(StreamId stream_id,
                                                 uint32_t to_read);

    outcome::result<kagome::common::Buffer> readSome(StreamId stream_id,
                                                     uint32_t to_read);

    void readAsync(
        StreamId stream_id,
        std::function<basic::Readable::BufferResultCallback> callback) noexcept;

    outcome::result<void> writeSome(StreamId stream_id,
                                    const kagome::common::Buffer &msg);

    outcome::result<void> write(StreamId stream_id,
                                const kagome::common::Buffer &msg);

    void writeAsync(
        StreamId stream_id, const kagome::common::Buffer &msg,
        std::function<basic::Writable::ErrorCodeCallback> handler) noexcept;

    /**
     * Half-close stream with a given id - means we will not send messages to
     * it, but they still can be received
     * @param stream_id of the stream to be closed
     */
    void closeStream(StreamId stream_id);

    bool streamCanRead(StreamId stream_id) const;

    bool streamCanWrite(StreamId stream_id) const;

    outcome::result<multi::Multiaddress> getRemoteMultiaddr() const;

    /// YamuxStream API ends

    /**
     * Get a stream id for a new stream
     * @return stream id
     */
    StreamId getNewStreamId();

    bool is_server_;
    transport::Connection &connection_;
    StreamId last_created_stream_id;

    /// as streams are full-duplex, there's a possibility we close the stream
    /// from out side and thus cannot write to it, but still can receive data
    std::set<StreamId> writable_streams_;
    std::set<StreamId> readable_streams_;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_LIBP2P_YAMUX_HPP
