/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_ADAPTOR_HPP
#define KAGOME_MUXER_ADAPTOR_HPP

#include <memory>

#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/connection.hpp"
#include "libp2p/transport/muxed_connection.hpp"

namespace libp2p::muxer {
  /**
   * Strategy to upgrade connections to muxed
   */
  struct MuxerAdaptor {
    virtual ~MuxerAdaptor() = default;
  };
}  // namespace libp2p::muxer

#endif  //KAGOME_MUXER_ADAPTOR_HPP
