/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/stream/yamux_stream.hpp"

namespace libp2p::stream {

  YamuxStream::YamuxStream(muxer::Yamux &yamux,
                           muxer::Yamux::StreamId stream_id)
      : yamux_{yamux}, stream_id_{stream_id} {}

  outcome::result<multi::Multiaddress> YamuxStream::getRemoteMultiaddr() const {
    return yamux_.getRemoteMultiaddr();
  }

  outcome::result<kagome::common::Buffer> YamuxStream::read(uint32_t to_read) {
    return yamux_.read(stream_id_, to_read);
  }

  outcome::result<kagome::common::Buffer> YamuxStream::readSome(
      uint32_t to_read) {
    return yamux_.readSome(stream_id_, to_read);
  }

  void YamuxStream::readAsync(
      std::function<BufferResultCallback> callback) noexcept {
    yamux_.readAsync(stream_id_, std::move(callback));
  }

  outcome::result<void> YamuxStream::writeSome(
      const kagome::common::Buffer &msg) {
    return yamux_.writeSome(stream_id_, msg);
  }

  outcome::result<void> YamuxStream::write(const kagome::common::Buffer &msg) {
    return yamux_.write(stream_id_, msg);
  }

  void YamuxStream::writeAsync(
      const kagome::common::Buffer &msg,
      std::function<ErrorCodeCallback> handler) noexcept {
    yamux_.writeAsync(stream_id_, msg, std::move(handler));
  }

  outcome::result<void> YamuxStream::close() {
    yamux_.closeStream(stream_id_);
    return outcome::success();
  }

  bool YamuxStream::isClosed() const {
    return yamux_.streamCanWrite(stream_id_);
  }

  bool YamuxStream::isClosedForRead() const {
    return yamux_.streamCanRead(stream_id_);
  }

}  // namespace libp2p::stream
