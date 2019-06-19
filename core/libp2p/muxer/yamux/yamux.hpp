/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_IMPL_HPP
#define KAGOME_YAMUX_IMPL_HPP

#include "libp2p/connection/yamux/yamux_config.hpp"
#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  class Yamux : public MuxerAdaptor {
   public:
    peer::Protocol getProtocolId() const noexcept override;

    outcome::result<std::shared_ptr<connection::CapableConnection>>
    muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                  StreamHandlerFunc handler,
                  connection::YamuxConfig config) const override;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_IMPL_HPP
