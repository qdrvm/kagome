/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include "libp2p/basic/readable.hpp"
#include "libp2p/basic/writable.hpp"

namespace libp2p::stream {
  /**
   * Stream between two peers in the network
   */
  class Stream : public basic::Writable, public basic::Readable {};
}  // namespace libp2p::stream

#endif  // KAGOME_STREAM_HPP
