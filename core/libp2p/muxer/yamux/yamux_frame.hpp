/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_FRAME_HPP
#define KAGOME_YAMUX_FRAME_HPP

#include <gsl/span>
#include "common/buffer.hpp"
#include "libp2p/muxer/yamux/yamux.hpp"

namespace libp2p::muxer {
  /**
   * Header with optional data, which is sent and accepted with Yamux protocol
   */
  struct YamuxFrame {
    using StreamId = Yamux::StreamId;
    static constexpr uint32_t kHeaderLength = 12;

    enum class FrameType : uint8_t {
      DATA = 0,           // transmit data
      WINDOW_UPDATE = 1,  // update the sender's receive window size
      PING = 2,           // ping for various purposes
      GO_AWAY = 3         // close the session
    };
    enum class Flag : uint16_t {
      SYN = 1,  // start of a new stream
      ACK = 2,  // acknowledge start of a new stream
      FIN = 4,  // half-close of the stream
      RST = 8   // reset a stream
    };
    enum class GoAwayError : uint32_t {
      NORMAL = 0,
      PROTOCOL_ERROR = 1,
      INTERNAL_ERROR = 2
    };
    static constexpr uint8_t kDefaultVersion = 0;
    static constexpr uint32_t kDefaultWindowSize = 256;

    uint8_t version_;
    FrameType type_;
    Flag flag_;
    StreamId stream_id_;
    uint32_t length_;
    kagome::common::Buffer data_;

    /**
     * Get bytes representation of the Yamux frame with given parameters
     * @return bytes of the frame
     */
    static kagome::common::Buffer frameBytes(
        uint8_t version, FrameType type, Flag flag, uint32_t stream_id,
        uint32_t length,
        const kagome::common::Buffer &data = kagome::common::Buffer{});
  };

  /**
   * Create a message, which notifies about a new stream creation
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer newStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message, which acknowledges a new stream creation
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer ackStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message, which closes a stream for writes
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer closeStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message, which resets a stream
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer resetStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message with an outcoming ping
   * @param value - ping value to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer pingOutMsg(uint32_t value);

  /**
   * Create a message, which responses to a ping
   * @param value - ping value to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer pingResponseMsg(uint32_t value);

  /**
   * Create a message with some data inside
   * @param stream_id to be put into the message
   * @param data to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer dataMsg(YamuxFrame::StreamId stream_id,
                                 const kagome::common::Buffer &data);

  /**
   * Create a message, which breaks a connection with a peer
   * @param error to be put into the message
   * @return bytes of the message
   */
  kagome::common::Buffer goAwayMsg(YamuxFrame::GoAwayError error);

  /**
   * Convert bytes into a frame object, if it is correct
   * @param frame_bytes to be converted
   * @return frame object, if convertation is successful, none otherwise
   */
  std::optional<YamuxFrame> parseFrame(gsl::span<const uint8_t> frame_bytes);
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_FRAME_HPP
