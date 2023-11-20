/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"

#include <fmt/format.h>

namespace kagome::storage {
  using namespace common::literals;

  inline const common::Buffer kRuntimeCodeKey = ":code"_buf;

  inline const common::Buffer kExtrinsicIndexKey = ":extrinsic_index"_buf;

  inline const common::Buffer kBlockTreeLeavesLookupKey =
      ":kagome:block_tree_leaves"_buf;

  inline const common::Buffer kActivePeersKey = ":kagome:last_active_peers"_buf;

  inline const common::Buffer kRuntimeHashesLookupKey =
      ":kagome:runtime_hashes"_buf;

  inline const common::Buffer kOffchainWorkerStoragePrefix = ":kagome:ocw"_buf;

  inline const common::Buffer kChildStoragePrefix = ":child_storage:"_buf;

  inline const common::Buffer kChildStorageDefaultPrefix =
      ":child_storage:default:"_buf;

  inline const common::Buffer kApplyingBlockInfoLookupKey =
      ":kagome:applying_block"_buf;

  inline const common::Buffer kWarpSyncCacheBlocksPrefix =
      ":kagome:WarpSyncCache:blocks:"_buf;

  inline const common::Buffer kWarpSyncOp = ":kagome:WarpSync:op"_buf;

  inline const common::Buffer kFirstBlockSlot = ":kagome:first_block_slot"_buf;

  inline const common::Buffer kBabeConfigRepositoryImplIndexerPrefix =
      ":kagome:BabeConfigRepositoryImpl:Indexer:"_buf;

  inline const common::Buffer kAuthorityManagerImplIndexerPrefix =
      ":kagome:AuthorityManagerImpl:Indexer:"_buf;

  inline const common::Buffer kRecentDisputeLookupKey = "recent_disputes"_buf;

  inline const common::Buffer kSessionsWindowLookupKey =
      "rolling_session_window"_buf;

  inline const common::Buffer kEarliestSessionLookupKey =
      "earliest-session"_buf;

  template <typename SessionT, typename CandidateHashT>
  inline common::Buffer kCandidateVotesLookupKey(
      SessionT session, const CandidateHashT &candidate) {
    return common::Buffer::fromString(
        fmt::format("candidate-votes:{:0>10}:{:l}", session, candidate));
  }

  /// Until what session have votes been cleaned up already?
  inline const common::Buffer kCleanedVotesWatermarkLookupKey =
      "cleaned-votes-watermark"_buf;

}  // namespace kagome::storage
