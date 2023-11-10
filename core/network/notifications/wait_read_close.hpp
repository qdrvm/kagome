/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/stream.hpp>

namespace kagome::network::notifications {
  using libp2p::connection::Stream;

  inline void waitReadClose(std::shared_ptr<Stream> stream) {
    auto buf = std::make_shared<std::vector<uint8_t>>(1);
    auto cb = [stream, buf](outcome::result<size_t> r) {
      if (r) {
        stream->reset();
        return;
      }
      stream->close([](outcome::result<void>) {});
    };
    stream->read(*buf, buf->size(), std::move(cb));
  }
}  // namespace kagome::network::notifications
