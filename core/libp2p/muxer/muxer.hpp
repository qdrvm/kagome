/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_HPP
#define KAGOME_MUXER_HPP

#include "libp2p/stream/stream.hpp"

namespace libp2p::muxer {
  /**
   * Muxer, allowing to create streams over one connection
   */
  class Muxer {
    /**
     * Create a new stream over the muxed connection
     * @return pointer to a created stream
     */
    virtual std::unique_ptr<stream::Stream> newStream() = 0;

    virtual ~Muxer() = 0;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXER_HPP
