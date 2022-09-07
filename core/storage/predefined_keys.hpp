/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
#define KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP

#include "common/buffer.hpp"

namespace kagome::storage {
  using namespace common::literals;

  inline const common::Buffer kRuntimeCodeKey = ":code"_buf;

  inline const common::Buffer kExtrinsicIndexKey = ":extrinsic_index"_buf;

  inline const common::Buffer kBlockTreeLeavesLookupKey =
      ":kagome:block_tree_leaves"_buf;

  inline const common::Buffer kActivePeersKey = ":kagome:last_active_peers"_buf;

  inline const common::Buffer kRuntimeHashesLookupKey =
      ":kagome:runtime_hashes"_buf;

  inline const common::Buffer kSchedulerTreeLookupKey =
      ":kagome:authorities:scheduler_tree"_buf;

  inline const common::Buffer kOffchainWorkerStoragePrefix = ":kagome:ocw"_buf;

  inline const common::Buffer kChildStorageDefaultPrefix =
      ":child_storage:default:"_buf;

  inline const common::Buffer kApplyingBlockInfoLookupKey =
      ":kagome:applying_block"_buf;

  inline const common::Buffer kAuthorityManagerStateLookupKey =
      ":kagome:authority_manager_state"_buf;

  inline const common::Buffer kBlockOfIncompleteSyncStateLookupKey =
      ":kagome:block_of_incomplete_sync_state"_buf;

}  // namespace kagome::storage

#endif  // KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
