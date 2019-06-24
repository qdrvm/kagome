/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_IMPL_HPP
#define KAGOME_YAMUX_IMPL_HPP

#include "libp2p/muxer/muxed_connection_config.hpp"
#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::connection {
  struct Stream;
}

namespace libp2p::muxer {
  class Yamux : public MuxerAdaptor {
   public:
    using StreamHandler =
        void(outcome::result<std::shared_ptr<connection::Stream>>);
    using StreamHandlerFunc = std::function<StreamHandler>;

    /**
     * Create a muxer with Yamux protocol
     * @param handler - function, which is called, when new streams arrive over
     * the connections this muxer muxed
     * @param config of muxers to be created over the connections
     */
    explicit Yamux(StreamHandlerFunc handler,
                   MuxedConnectionConfig config = MuxedConnectionConfig{});

    peer::Protocol getProtocolId() const noexcept override;

    outcome::result<std::shared_ptr<connection::CapableConnection>>
    muxConnection(
        std::shared_ptr<connection::SecureConnection> conn) const override;

   private:
    StreamHandlerFunc handler_;
    MuxedConnectionConfig config_;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_IMPL_HPP
