/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_HPP
#define KAGOME_YAMUX_HPP

#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  class YamuxAdaptor : public MuxerAdaptor {  // will be renamed
   public:
    ~YamuxAdaptor() override = default;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_HPP
