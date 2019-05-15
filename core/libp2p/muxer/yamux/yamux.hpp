/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_IMPL_HPP
#define KAGOME_YAMUX_IMPL_HPP

#include "libp2p/muxer/stream_muxer.hpp"

#include "libp2p/muxer/yamux/yamux_config.hpp"

namespace libp2p::muxer {
  /**
   * Strategy to upgrade connections to Yamuxed ones
   */
  class Yamux : public StreamMuxer {
   public:
    /**
     * Create a Yamux strategy
     * @param config, with which it is to be created
     */
    explicit Yamux(YamuxConfig config);

    Yamux(const Yamux &other) = default;
    Yamux &operator=(const Yamux &other) = default;
    Yamux(Yamux &&other) noexcept = default;
    Yamux &operator=(Yamux &&other) noexcept = default;
    ~Yamux() override = default;

    std::unique_ptr<transport::MuxedConnection> upgrade(
        std::shared_ptr<transport::Connection> connection,
        NewStreamHandler stream_handler) const override;

   private:
    YamuxConfig config_;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_IMPL_HPP
