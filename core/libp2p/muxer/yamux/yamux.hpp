/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_HPP
#define KAGOME_LIBP2P_YAMUX_HPP

#include <queue>
#include <set>

#include <outcome/outcome.hpp>
#include "libp2p/muxer/muxer.hpp"
#include "libp2p/transport/connection.hpp"
#include "yamux_config.hpp"

namespace libp2p::stream {
  class Stream;
  class YamuxStream;
}  // namespace libp2p::stream

namespace libp2p::muxer {
  struct YamuxFrame;

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class Yamux : public Muxer {
    friend class libp2p::stream::YamuxStream;  // to use private stream API
    friend struct YamuxFrame;                  // to provide StreamId to it

    /// according to spec, there is a 32-bit number for stream id
    using StreamId = uint32_t;

   public:
    /**
     * Create a Yamux multiplexer
     * @param connection over which it should be created
     * @param config - parameters of this instance
     */
    Yamux(transport::Connection &connection, YamuxConfig config);

    ~Yamux() override;

    /**
     * Spawn a new stream over the underlying connection
     * @return pointer to new stream
     */
    std::unique_ptr<stream::Stream> newStream() override;

    /**
     * Get a multiaddress, over which this instance is created
     * @return result with the multiaddress or error
     */
    outcome::result<multi::Multiaddress> getRemoteMultiaddr() const;

   private:
    /**
     * Get a stream id for a new stream
     * @return stream id
     */
    StreamId getNewStreamId();

    /**
     * Process a frame, received from the stream
     * @param frame to be processed
     * @param stream_id - stream, for which this frame was supposed to be
     * @return true, if the message was just processed, false if the underlying
     * connection was closed
     */
    bool processFrame(const YamuxFrame &frame);

    /**
     * Get the message for a given stream if it was buffered
     * @param stream_id to be looked for
     * @return optional with the message
     */
    std::optional<common::NetworkMessage> findMsgInBuffer(StreamId stream_id);

    /**
     * Close the underlying connection
     */
    void closeConnection();

    /**
     * Close the stream and remove all mentions about it
     * @param stream_id to be closed
     */
    void removeStream(StreamId stream_id);

    /**
     * Close a stream for reads
     * @param stream_id to be closed
     */
    void closeStreamForRead(StreamId stream_id);

    /**
     * Close a stream for writes
     * @param stream_id to be closed
     */
    void closeStreamForWrite(StreamId stream_id);

    /**
     * Reset all streams, which can be written to
     */
    void resetAllStreams();

    /// YamuxStream API begins

    enum class YamuxError { kConnectionError = 1, kNoWrites, kNoReads };

    /**
     * Read a frame from a given stream
     * @param stream_id of the stream to be read from
     * @return result with a network message or error
     * @note stream must be opened for reads
     */
    outcome::result<common::NetworkMessage> readFrame(StreamId stream_id);

    /**
     * Write a message to a given stream
     * @param stream_id of the stream to be written to
     * @param msg to be written
     * @return result with void in case of success or with error otherwise
     * @note stream must be opened for writes
     */
    outcome::result<void> writeFrame(StreamId stream_id,
                                     const common::NetworkMessage &msg);

    /**
     * Half-close stream with a given id - means we will not send messages to
     * it, but they still can be received
     * @param stream_id of the stream to be closed
     */
    void closeStream(StreamId stream_id);

    /**
     * Close the stream entirely: neither writes nor reads are possible from
     * that point
     * @param stream_id of the stream to be reset
     */
    void resetStream(StreamId stream_id);

    /**
     * Check, if the stream can read
     * @param stream_id to be checked
     * @return true, if stream can read, false otherwise
     */
    bool streamCanRead(StreamId stream_id) const;

    /**
     * Check, if the stream can write
     * @param stream_id to be checked
     * @return true, if stream can write, false otherwise
     */
    bool streamCanWrite(StreamId stream_id) const;

    /// YamuxStream API ends

    const bool is_server_;
    transport::Connection &connection_;  // shared_ptr should be
    StreamId last_created_stream_id;

    /// as streams are full-duplex, there's a possibility we close the stream
    /// from out side and thus cannot write to it, but still can receive data
    std::set<StreamId> writable_streams_;
    std::set<StreamId> readable_streams_;

    /// buffers for received messages for streams
    std::map<StreamId, std::queue<common::NetworkMessage>> streams_buffers_;

    // FIXME: no window size management is done for now
    /// window sizes per buffer
    std::map<StreamId, uint32_t> streams_windows_;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_LIBP2P_YAMUX_HPP
