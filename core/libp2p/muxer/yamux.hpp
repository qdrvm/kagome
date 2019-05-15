/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_HPP
#define KAGOME_YAMUX_HPP

#include "libp2p/muxer/stream_muxer.hpp"

namespace libp2p::muxer {

  // adapter
  class Yamux : public StreamMuxer {
   public:
    ~Yamux() override = default;
  };

}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_HPP
