/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNC_CLIENTS_SET_HPP
#define KAGOME_SYNC_CLIENTS_SET_HPP

#include <memory>
#include <unordered_set>

#include "network/sync_protocol_client.hpp"

namespace kagome::network {
  /**
   * Keeps all known Sync clients
   */
  struct SyncClientsSet {
    std::vector<std::shared_ptr<SyncProtocolClient>> clients;
  };
}  // namespace kagome::network

#endif  // KAGOME_SYNC_CLIENTS_SET_HPP
