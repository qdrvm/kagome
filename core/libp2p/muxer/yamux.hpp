/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_HPP
#define KAGOME_YAMUX_HPP

#include "libp2p/muxer/yamux/yamux.hpp"
#include "libp2p/muxer/yamux/yamux_config.hpp"

#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {

  // adapter
  // TODO: rename to Yamux
  class Yamux2 : public MuxerAdaptor {
   public:
    ~Yamux2() override = default;
  };

}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_HPP
