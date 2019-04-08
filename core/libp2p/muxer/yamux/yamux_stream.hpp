/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_STREAM_HPP
#define KAGOME_LIBP2P_YAMUX_STREAM_HPP

#include "libp2p/muxer/yamux/yamux.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::stream {
  /**
   * Stream implementation, used by Yamux multiplexer
   */
  class YamuxStream : public Stream {
   public:
    YamuxStream(muxer::Yamux &yamux, muxer::Yamux::StreamId stream_id);

    outcome::result<common::NetworkMessage> readFrame() override;

    outcome::result<void> writeFrame(
        const common::NetworkMessage &msg) override;

    void close() override;

    void reset() override;

    bool isClosedForWrite() const override;

    bool isClosedForRead() const override;

    bool isClosedEntirely() const override;

    ~YamuxStream() override;

   private:
    muxer::Yamux &yamux_;
    muxer::Yamux::StreamId stream_id_;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_YAMUX_STREAM_HPP
