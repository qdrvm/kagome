/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "yamux_stream.hpp"

namespace libp2p::stream {

  YamuxStream::YamuxStream(std::shared_ptr<muxer::Yamux> yamux,
                           muxer::Yamux::StreamId stream_id)
      : yamux_{std::move(yamux)}, stream_id_{stream_id} {}

  YamuxStream::~YamuxStream() {
    this->resetStream();
  }

  void YamuxStream::readAsync(ReadCompletionHandler completion_handler) {
    yamux_->streamReadFrameAsync(stream_id_, std::move(completion_handler));
  }

  void YamuxStream::writeAsync(const kagome::common::Buffer &msg) {
    yamux_->streamWriteFrameAsync(
        stream_id_, msg,
        [](std::error_code /* unused */, size_t /* unused */) {});
  }

  void YamuxStream::writeAsync(const kagome::common::Buffer &msg,
                               ErrorCodeCallback error_callback) {
    yamux_->streamWriteFrameAsync(stream_id_, msg, std::move(error_callback));
  }

  void YamuxStream::close() {
    yamux_->streamClose(stream_id_);
  }

  void YamuxStream::reset() {
    resetStream();
  }

  bool YamuxStream::isClosedForWrite() const {
    return yamux_->streamIsClosedForWrite(stream_id_);
  }

  bool YamuxStream::isClosedForRead() const {
    return yamux_->streamIsClosedForRead(stream_id_);
  }

  bool YamuxStream::isClosedEntirely() const {
    return yamux_->streamIsClosedEntirely(stream_id_);
  }

  void YamuxStream::resetStream() {
    yamux_->streamReset(stream_id_);
  }

}  // namespace libp2p::stream
