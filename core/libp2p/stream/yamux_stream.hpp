/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_STREAM_HPP
#define KAGOME_LIBP2P_YAMUX_STREAM_HPP

#include "libp2p/stream/stream.hpp"

namespace libp2p::stream {
  class libp2p::muxer::Yamux;

  /**
   * Stream implementation, used by Yamux multiplexer
   */
  class YamuxStream : public Stream {
   public:
    YamuxStream(const muxer::Yamux &yamux, muxer::Yamux::StreamId stream_id);

    void write(const common::NetworkMessage &msg) const override;
    boost::optional<common::NetworkMessage> read() const override;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_YAMUX_STREAM_HPP
