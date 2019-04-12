/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_HPP
#define KAGOME_YAMUX_HPP

#include <map>
#include <queue>

#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>
#include "common/logger.hpp"
#include "libp2p/common/network_message.hpp"
#include "libp2p/muxer/yamux/yamux_config.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/connection.hpp"
#include "libp2p/transport/muxed_connection.hpp"

namespace libp2p::stream {
  class YamuxStream;
}

namespace libp2p::muxer {
  class YamuxFrame;

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class Yamux : public transport::MuxedConnection,
                public std::enable_shared_from_this<Yamux> {
   public:
    using StreamId = uint32_t;
    using NewStreamHandler =
        std::function<void(std::unique_ptr<stream::Stream>)>;

    enum class YamuxErrorStream {
      NO_SUCH_STREAM = 1,
      NOT_WRITABLE,
      NOT_READABLE,
      YAMUX_IS_CLOSED
    };

    /**
     * Create a new Yamux instance
     * @param connection, multiplexed by this instance
     * @param stream_handler - function, which is going to be called, when a new
     * stream arrives
     * @param yamux_config to configure this instance
     */
    Yamux(
        std::shared_ptr<transport::Connection> connection,
        NewStreamHandler stream_handler, YamuxConfig yamux_config,
        kagome::common::Logger logger = kagome::common::createLogger("Yamux"));

    ~Yamux() override;

    void start() override;

    void stop() override;

    outcome::result<std::unique_ptr<stream::Stream>> newStream() override;

    void close() override;

    bool isClosed() override;

   private:
    /**
     * Helper function, which closes the Yamux; needed in order to void
     * "calling virtual functions from destructor"
     */
    void closeYamux();

    struct StreamParameters;

    void startReadingHeader();

    void readingHeaderCompleted(const std::error_code &ec, size_t n);

    void readingDataCompleted(const std::error_code &ec, size_t n,
                              StreamParameters &stream);

    void write(const common::NetworkMessage &msg,
               stream::Stream::ErrorCodeCallback cb);

    void startWriting();

    void writingCompleted(
        const std::error_code &ec, size_t n,
        const stream::Stream::ErrorCodeCallback &error_callback);

    /**
     * Get a stream id, with which a new stream is to be created
     * @return new id
     */
    StreamId getNewStreamId();

    /**
     * Register a new stream in this instance, making it active
     * @param stream_id to be registered
     */
    void registerNewStream(StreamId stream_id);

    /**
     * If there is data in this length, buffer it to the according stream
     * @param stream for the data to be inserted in
     * @param frame, which can have some data inside
     * @return true, if it is going to initiate new iteration of Yamux event
     * loop itself, false if caller should do it
     */
    bool processData(std::shared_ptr<Yamux::StreamParameters> stream,
                     const YamuxFrame &frame);

    /**
     * Process ack message for such stream_id
     * @param stream_id of the stream to be processed
     * @return stream, if it is opened on this side, none otherwise
     */
    std::optional<std::shared_ptr<StreamParameters>> processAck(
        StreamId stream_id);

    /**
     * Find stream with such id in local streams
     * @param stream_id to be found
     * @return stream, if it is opened on this side, none otherwise
     */
    std::optional<std::shared_ptr<StreamParameters>> findStream(
        StreamId stream_id) const;

    /**
     * Close stream for reads on this side
     * @param stream_id to be closed
     */
    void closeStreamForRead(StreamId stream_id);

    /**
     * Close stream for writes from this side
     * @param stream_id to be closed
     */
    void closeStreamForWrite(StreamId stream_id);

    /**
     * Close stream entirely
     * @param stream_id to be closed
     */
    void removeStream(StreamId stream_id);

    /**
     * Reset all streams, which this instance is multiplexing
     */
    void resetAllStreams() noexcept;

    /**
     * Process bytes, which must be a YamuxFrame header
     * @return true, if it is going to initiate new iteration of Yamux event
     * loop itself, false if caller should do it
     */
    bool processHeader();

    /**
     * Process frame of data type
     * @param frame to be processed
     * @return true, if it is going to initiate new iteration of Yamux event
     * loop itself, false if caller should do it
     */
    bool processDataFrame(const YamuxFrame &frame);

    /**
     * Process frame of window size update type
     * @param frame to be processed
     */
    void processWindowUpdateFrame(const YamuxFrame &frame);

    /**
     * Process frame of ping type
     * @param frame to be processed
     */
    void processPingFrame(const YamuxFrame &frame);

    /**
     * Process frame of go away type
     * @param frame to be processed
     */
    void processGoAwayFrame(const YamuxFrame &frame);

    std::shared_ptr<transport::Connection> connection_;
    NewStreamHandler new_stream_handler_;
    bool is_active_;
    uint32_t last_created_stream_id_;
    YamuxConfig config_;

    boost::asio::streambuf read_buffer_;
    kagome::common::Buffer write_buffer_;
    bool is_writing_ = false;

    struct StreamParameters {
      bool is_readable_;
      bool is_writable_;
      uint32_t window_size_;

      std::queue<common::NetworkMessage> buffered_messages_{};
      std::queue<stream::Stream::ReadCompletionHandler> completion_handlers_{};
    };
    /// streams, which are multiplexed by this Yamux instance
    std::map<StreamId, std::shared_ptr<StreamParameters>> streams_;

    /// messages, which are going to be written during the event loop execution
    using MsgAndCallback =
        std::pair<common::NetworkMessage, stream::Stream::ErrorCodeCallback>;
    std::queue<MsgAndCallback> outcoming_messages_{};

    kagome::common::Logger logger_;

    /// YAMUX STREAM API

    friend class stream::YamuxStream;

    void streamReadFrameAsync(
        StreamId stream_id,
        stream::Stream::ReadCompletionHandler completion_handler);

    void streamWriteFrameAsync(
        StreamId stream_id, const common::NetworkMessage &msg,
        stream::Stream::ErrorCodeCallback error_callback);

    void streamClose(StreamId stream_id);

    void streamReset(StreamId stream_id);

    bool streamIsClosedForWrite(StreamId stream_id) const;

    bool streamIsClosedForRead(StreamId stream_id) const;

    bool streamIsClosedEntirely(StreamId stream_id) const;
  };
}  // namespace libp2p::muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::muxer, Yamux::YamuxErrorStream)

#endif  // KAGOME_YAMUX_HPP
