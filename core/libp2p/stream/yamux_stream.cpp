/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/stream/yamux_stream.hpp"

namespace libp2p::stream {

  YamuxStream::YamuxStream(muxer::Yamux &yamux,
                           muxer::Yamux::StreamId stream_id)
      : yamux_{yamux}, stream_id_{stream_id} {}

  outcome::result<common::NetworkMessage> YamuxStream::readFrame() {
    return yamux_.readFrame(stream_id_);
  }

  outcome::result<void> YamuxStream::writeFrame(
      const common::NetworkMessage &msg) {
    return yamux_.writeFrame(stream_id_, msg);
  }

  void YamuxStream::close() {
    yamux_.closeStream(stream_id_);
  }

  void YamuxStream::reset() {
    yamux_.resetStream(stream_id_);
  }

  bool YamuxStream::isClosedForWrite() const {
    return yamux_.streamCanWrite(stream_id_);
  }

  bool YamuxStream::isClosedForRead() const {
    return yamux_.streamCanRead(stream_id_);
  }

  YamuxStream::~YamuxStream() {
    this->reset();
  }

}  // namespace libp2p::stream
