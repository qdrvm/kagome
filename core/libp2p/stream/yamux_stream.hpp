/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_STREAM_HPP
#define KAGOME_LIBP2P_YAMUX_STREAM_HPP

#include "libp2p/muxer/yamux.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::stream {
  /**
   * Stream implementation, used by Yamux multiplexer
   */
  class YamuxStream : public Stream {
   public:
    YamuxStream(muxer::Yamux &yamux, muxer::Yamux::StreamId stream_id);

    outcome::result<multi::Multiaddress> getRemoteMultiaddr() const override;

    outcome::result<kagome::common::Buffer> read(uint32_t to_read) override;

    outcome::result<kagome::common::Buffer> readSome(uint32_t to_read) override;

    void readAsync(
        std::function<BufferResultCallback> callback) noexcept override;

    outcome::result<void> writeSome(const kagome::common::Buffer &msg) override;

    outcome::result<void> write(const kagome::common::Buffer &msg) override;

    void writeAsync(const kagome::common::Buffer &msg,
                    std::function<ErrorCodeCallback> handler) noexcept override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    bool isClosedForRead() const override;

    ~YamuxStream() override = default;

   private:
    muxer::Yamux &yamux_;
    muxer::Yamux::StreamId stream_id_;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_YAMUX_STREAM_HPP
