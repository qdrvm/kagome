/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_MESSAGE_HPP
#define KAGOME_NETWORK_MESSAGE_HPP

#include "common/buffer.hpp"

namespace libp2p::common {
  /**
   * Encoded message, which is sent through the network
   */
    using NetworkMessage = kagome::common::Buffer;
}  // namespace libp2p::common

#endif  // KAGOME_NETWORK_MESSAGE_HPP
