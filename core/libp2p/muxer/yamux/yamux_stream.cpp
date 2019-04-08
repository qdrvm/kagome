/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "yamux_stream.hpp"

namespace libp2p::stream {

  YamuxStream::YamuxStream(muxer::Yamux &yamux,
                           muxer::Yamux::StreamId stream_id)
      : yamux_{yamux}, stream_id_{stream_id} {}

  YamuxStream::~YamuxStream() {
    this->reset();
  }

  outcome::result<common::NetworkMessage> YamuxStream::readFrame() {
    return yamux_.streamReadFrame(stream_id_);
  }

  outcome::result<void> YamuxStream::writeFrame(
      const common::NetworkMessage &msg) {
    return yamux_.streamWriteFrame(stream_id_, msg);
  }

  void YamuxStream::close() {
    yamux_.streamClose(stream_id_);
  }

  void YamuxStream::reset() {
    yamux_.streamReset(stream_id_);
  }

  bool YamuxStream::isClosedForWrite() const {
    return yamux_.streamIsClosedForWrite(stream_id_);
  }

  bool YamuxStream::isClosedForRead() const {
    return yamux_.streamIsClosedForRead(stream_id_);
  }

  bool YamuxStream::isClosedEntirely() const {
    return yamux_.streamIsClosedEntirely(stream_id_);
  }

}  // namespace libp2p::stream
