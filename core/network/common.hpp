/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_COMMON_HPP
#define KAGOME_NETWORK_COMMON_HPP

#include "libp2p/peer/protocol.hpp"

namespace kagome::network {
  /// Current protocol version.
  static constexpr uint32_t CURRENT_VERSION = 6;
  /// Lowest version we support
  static constexpr uint32_t MIN_VERSION = 3;

  const libp2p::peer::Protocol kSyncProtocol = "/{}/sync/2";
  const libp2p::peer::Protocol kPropagateTransactionsProtocol =
      "/{}/transactions/1";
  const libp2p::peer::Protocol kBlockAnnouncesProtocol =
      "/{}/block-announces/1";
  const libp2p::peer::Protocol kGrandpaProtocol = "/paritytech/grandpa/1";
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_COMMON_HPP
