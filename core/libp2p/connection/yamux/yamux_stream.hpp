/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_YAMUX_STREAM_HPP
#define KAGOME_LIBP2P_YAMUX_STREAM_HPP

#include "yamuxed_connection.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::stream {
  /**
   * Stream implementation, used by Yamux multiplexer
   */
  class YamuxStream : public Stream {
   public:
    YamuxStream(std::shared_ptr<muxer::Yamux> yamux,
                muxer::Yamux::StreamId stream_id);

    YamuxStream(const YamuxStream &other) = delete;
    YamuxStream &operator=(const YamuxStream &other) = delete;

    YamuxStream(YamuxStream &&other) noexcept = default;
    YamuxStream &operator=(YamuxStream &&other) noexcept = default;

    void readAsync(ReadCompletionHandler completion_handler) const override;

    void writeAsync(const kagome::common::Buffer &msg) const override;

    void writeAsync(const kagome::common::Buffer &msg,
                    ErrorCodeCallback error_callback) const override;

    void close() override;

    void reset() override;

    bool isClosedForWrite() const override;

    bool isClosedForRead() const override;

    bool isClosedEntirely() const override;

    ~YamuxStream() override;

   private:
    std::shared_ptr<muxer::Yamux> yamux_;
    muxer::Yamux::StreamId stream_id_;

    /**
     * Helper function, which resets the stream; needed in order to void
     * "calling virtual functions from destructor"
     */
    void resetStream();
  };
}  // namespace libp2p::stream

#endif  // KAGOME_YAMUX_STREAM_HPP
