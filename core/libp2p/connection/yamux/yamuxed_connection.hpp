/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUXED_CONNECTION_HPP
#define KAGOME_YAMUXED_CONNECTION_HPP

#include <functional>
#include <map>

#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::connection {
  class YamuxStream;
  struct YamuxFrame;

  /**
   * Implementation of multiplexed connection - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class YamuxedConnection
      : public CapableConnection,
        public std::enable_shared_from_this<YamuxedConnection> {
   public:
    using StreamId = uint32_t;
    using NewStreamHandler = std::function<Stream::Handler>;

    enum class Error {
      NO_SUCH_STREAM = 1,
      YAMUX_IS_CLOSED,
      FORBIDDEN_CALL,
      OTHER_SIDE_ERROR,
      INTERNAL_ERROR
    };

    /**
     * Create a new Yamux instance
     * @param connection to be multiplexed
     * @param stream_handler - function, which is going to be called, when a new
     * stream arrives
     * @param logger - logger to log execution details
     */
    YamuxedConnection(
        std::shared_ptr<SecureConnection> connection,
        NewStreamHandler stream_handler,
        kagome::common::Logger logger = kagome::common::createLogger("Yamux"));

    YamuxedConnection(const YamuxedConnection &other) = delete;
    YamuxedConnection &operator=(const YamuxedConnection &other) = delete;
    YamuxedConnection(YamuxedConnection &&other) noexcept = delete;
    YamuxedConnection &operator=(YamuxedConnection &&other) noexcept = delete;
    ~YamuxedConnection() override = default;

    outcome::result<std::shared_ptr<Stream>> newStream() override;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    /// part of connection API, which use is forbidden - client may only use
    /// streams to communicate over the multiplexed connection
    outcome::result<size_t> write(gsl::span<const uint8_t> in) override;
    outcome::result<size_t> writeSome(gsl::span<const uint8_t> in) override;
    outcome::result<std::vector<uint8_t>> read(size_t bytes) override;
    outcome::result<std::vector<uint8_t>> readSome(size_t bytes) override;
    outcome::result<size_t> read(gsl::span<uint8_t> buf) override;
    outcome::result<size_t> readSome(gsl::span<uint8_t> buf) override;

    /**
     * Read and process one frame
     * @return nothing or error
     */
    outcome::result<void> readFrame();

    /**
     * Process frame of data type
     * @param frame to be processed
     * @return nothing on success, error otherwise
     */
    outcome::result<void> processDataFrame(const YamuxFrame &frame);

    /**
     * Process frame of window size update type
     * @param frame to be processed
     */
    outcome::result<void> processWindowUpdateFrame(const YamuxFrame &frame);

    /**
     * Process frame of ping type
     * @param frame to be processed
     */
    outcome::result<void> processPingFrame(const YamuxFrame &frame);

    /**
     * Process frame of go away type
     * @param frame to be processed
     */
    outcome::result<void> processGoAwayFrame(const YamuxFrame &frame);

    /**
     * Find stream with such id in local streams
     * @param stream_id to be found
     * @return stream, if it is opened on this side, nullptr otherwise
     */
    std::shared_ptr<YamuxStream> findStream(StreamId stream_id);

    /**
     * Register a new stream in this instance, making it active
     * @param stream_id to be registered
     * @return registered stream or error
     */
    outcome::result<std::shared_ptr<YamuxStream>> registerNewStream(
        StreamId stream_id);

    /**
     * If there is data in this length, buffer it to the according stream
     * @param stream, for which the data arrived
     * @param frame, which can have some data inside
     * @return nothing on success, error ottherwise
     */
    outcome::result<void> processData(
        const std::shared_ptr<YamuxStream> &stream, const YamuxFrame &frame);

    /**
     * Process ack message for such stream_id
     * @param stream_id of the stream to be processed
     * @return stream, if it is opened on this side, none otherwise
     */
    outcome::result<std::shared_ptr<YamuxStream>> processAck(
        StreamId stream_id);

    /**
     * Close stream for reads on this side
     * @param stream_id to be closed
     */
    void closeStreamForRead(StreamId stream_id);

    /**
     * Close stream for writes from this side
     * @param stream_id to be closed
     * @return nothing or error
     */
    outcome::result<void> closeStreamForWrite(StreamId stream_id);

    /**
     * Close stream entirely
     * @param stream_id to be closed
     */
    void removeStream(StreamId stream_id);

    /**
     * Get a stream id, with which a new stream is to be created
     * @return new id
     */
    StreamId getNewStreamId();

    std::shared_ptr<SecureConnection> connection_;

    NewStreamHandler new_stream_handler_;
    uint32_t last_created_stream_id_;
    bool is_active_;
    std::map<StreamId, std::shared_ptr<YamuxStream>> streams_;

    kagome::common::Logger logger_;

    /// YAMUX STREAM API

    friend class YamuxStream;

    outcome::result<void> streamProcessNextFrame();

    outcome::result<size_t> streamWrite(StreamId stream_id,
                                        gsl::span<const uint8_t> msg,
                                        bool some);

    outcome::result<void> streamAckBytes(StreamId stream_id, uint32_t bytes);

    outcome::result<void> streamClose(StreamId stream_id);

    void streamReset(StreamId stream_id);
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, YamuxedConnection::Error)

#endif  // KAGOME_YAMUXED_CONNECTION_HPP
