/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
#define KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP

#include "common/buffer.hpp"

namespace kagome::storage {

  inline const common::Buffer kAuthoritySetKey =
      common::Buffer().put("grandpa_voters");
  inline const common::Buffer kSetStateKey =
      common::Buffer().put("grandpa_completed_round");

  inline const common::Buffer kGenesisBlockHashLookupKey =
      common::Buffer().put(":kagome:genesis_block_hash");
  inline const common::Buffer kLastFinalizedBlockHashLookupKey =
      common::Buffer().put(":kagome:last_finalized_block_hash");

  inline const common::Buffer kLastBabeEpochNumberLookupKey =
      common::Buffer().put(":kagome:last_babe_epoch_number");

  inline const common::Buffer kActivePeersKey =
      common::Buffer().put(":kagome:last_active_peers");

  inline const common::Buffer kSchedulerTreeLookupKey =
      common::Buffer{}.put(":kagome:authorities:scheduler_tree");
}  // namespace kagome::storage

#endif  // KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
