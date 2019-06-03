/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_STREAM_HPP
#define KAGOME_LIBP2P_YAMUX_STREAM_HPP

#include <boost/asio/streambuf.hpp>
#include "libp2p/connection/stream.hpp"
#include "libp2p/connection/yamux/yamuxed_connection.hpp"

namespace libp2p::connection {
  /**
   * Stream implementation, used by Yamux multiplexer
   */
  class YamuxStream : public Stream {
   public:
    YamuxStream(std::shared_ptr<YamuxedConnection> conn,
                YamuxedConnection::StreamId stream_id);

    YamuxStream(const YamuxStream &other) = delete;
    YamuxStream &operator=(const YamuxStream &other) = delete;
    YamuxStream(YamuxStream &&other) noexcept = delete;
    YamuxStream &operator=(YamuxStream &&other) noexcept = delete;
    ~YamuxStream() override;

    outcome::result<size_t> write(gsl::span<const uint8_t> in) override;

    outcome::result<size_t> writeSome(gsl::span<const uint8_t> in) override;

    outcome::result<std::vector<uint8_t>> read(size_t bytes) override;

    outcome::result<std::vector<uint8_t>> readSome(size_t bytes) override;

    outcome::result<size_t> read(gsl::span<uint8_t> buf) override;

    outcome::result<size_t> readSome(gsl::span<uint8_t> buf) override;

    void reset() override;

    bool isClosedForRead() const override;

    bool isClosedForWrite() const override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    friend class YamuxedConnection;

    std::shared_ptr<YamuxedConnection> yamux_;
    YamuxedConnection::StreamId stream_id_;

    /// is the stream closed for reads?
    bool is_readable_;

    /// is the stream closed for writes?
    bool is_writable_;

    /// window size of the stream - how much unacknowledged bytes can be
    uint32_t window_size_;

    /// buffer with bytes, not consumed by this stream
    boost::asio::streambuf read_buffer_;

    /// buffer with bytes, which cannot be sent because of the window
    boost::asio::streambuf write_buffer_;

    void resetStream();

    void commitData(std::vector<uint8_t> &&data);
  };
}  // namespace libp2p::connection

#endif  // KAGOME_YAMUX_STREAM_HPP
