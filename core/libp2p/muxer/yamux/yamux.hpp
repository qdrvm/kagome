/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_ADAPTOR_HPP
#define KAGOME_YAMUX_ADAPTOR_HPP

#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  class Yamux : public MuxerAdaptor {
   public:
    peer::Protocol getProtocolId() const override;

    outcome::result<std::shared_ptr<connection::CapableConnection>>
    muxConnection(
        std::shared_ptr<connection::SecureConnection> conn) const override;

    ~Yamux() override = default;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_ADAPTOR_HPP
