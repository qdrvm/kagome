/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_IMPL_HPP
#define KAGOME_YAMUX_IMPL_HPP

#include <functional>
#include <map>
#include <queue>

#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/muxed_connection.hpp"
#include "yamux_config.hpp"
#include "yamux_stream_parameters.hpp"

namespace libp2p::stream {
  class YamuxStream;
}

namespace libp2p::connection {
  struct YamuxFrame;

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class YamuxedConnection : public transport::MuxedConnection {
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
    YamuxedConnection(
        std::shared_ptr<transport::Connection> connection,
        NewStreamHandler stream_handler, YamuxConfig yamux_config,
        kagome::common::Logger logger = kagome::common::createLogger("Yamux"));

    YamuxedConnection(const YamuxedConnection &other) = delete;
    YamuxedConnection &operator=(const YamuxedConnection &other) = delete;
    YamuxedConnection(YamuxedConnection &&other) noexcept = delete;
    YamuxedConnection &operator=(YamuxedConnection &&other) noexcept = delete;

    ~YamuxedConnection() override;

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

    void startReadingHeader();

    void readingHeaderCompleted(const std::error_code &ec, size_t n);

    void readingDataCompleted(
        const std::error_code &ec, size_t n,
        std::reference_wrapper<YamuxStreamParameters> stream_wrapper);

    void write(const kagome::common::Buffer &msg,
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
    bool processData(std::reference_wrapper<YamuxStreamParameters> stream,
                     const YamuxFrame &frame);

    /**
     * Process ack message for such stream_id
     * @param stream_id of the stream to be processed
     * @return stream, if it is opened on this side, none otherwise
     */
    std::optional<std::reference_wrapper<YamuxStreamParameters>> processAck(
        StreamId stream_id);

    /**
     * Find stream with such id in local streams
     * @param stream_id to be found
     * @return stream, if it is opened on this side, none otherwise
     */
    std::optional<std::reference_wrapper<YamuxStreamParameters>> findStream(
        StreamId stream_id);

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

    /// streams, which are multiplexed by this Yamux instance
    std::map<StreamId, YamuxStreamParameters> streams_;

    /// messages, which are going to be written during the event loop execution
    using MsgAndCallback =
        std::pair<kagome::common::Buffer, stream::Stream::ErrorCodeCallback>;
    std::queue<MsgAndCallback> outcoming_messages_{};

    kagome::common::Logger logger_;

    /// YAMUX STREAM API

    friend class stream::YamuxStream;

    void streamReadFrameAsync(
        StreamId stream_id,
        stream::Stream::ReadCompletionHandler completion_handler);

    void streamWriteFrameAsync(
        StreamId stream_id, const kagome::common::Buffer &msg,
        stream::Stream::ErrorCodeCallback error_callback);

    void streamClose(StreamId stream_id);

    void streamReset(StreamId stream_id);

    bool streamIsClosedForWrite(StreamId stream_id);

    bool streamIsClosedForRead(StreamId stream_id);

    bool streamIsClosedEntirely(StreamId stream_id);
  };
}  // namespace libp2p::muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::muxer, Yamux::YamuxErrorStream)

#endif  // KAGOME_YAMUX_IMPL_HPP
