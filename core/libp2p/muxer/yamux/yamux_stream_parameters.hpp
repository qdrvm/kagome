/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_STREAM_PARAMETERS_HPP
#define KAGOME_YAMUX_STREAM_PARAMETERS_HPP

#include "common/buffer.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::muxer {

  /**
   * Run-time parameters of Yamux stream
   */
  struct YamuxStreamParameters {
    /// is the stream closed for reads?
    bool is_readable_;

    /// is the stream closed for writes?
    bool is_writable_;

    /// window size of the stream - how much unacknowledged bytes can be
    uint32_t window_size_;

    /// messages, came for that stream, but not read yet
    std::queue<kagome::common::Buffer> buffered_messages_{};

    /// handlers for messages, which arrive for that stream
    std::queue<stream::Stream::ReadCompletionHandler> completion_handlers_{};
  };

}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_STREAM_PARAMETERS_HPP
