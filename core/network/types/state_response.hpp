/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_RESPONSE_HPP
#define KAGOME_STATE_RESPONSE_HPP

#include "common/buffer.hpp"
#include "primitives/common.hpp"
#include "storage/trie/types.hpp"

namespace kagome::network {
  /// A key-value pair.
  struct StateEntry {
    common::Buffer key;
    common::Buffer value;
  };

  /// A key value state.
  struct KeyValueStateEntry {
    /// Root of for this level, empty length bytes
    /// if top level.
    storage::trie::RootHash state_root;
    /// A collection of keys-values.
    std::vector<StateEntry> entries;
    /// Set to true when there are no more keys to return.
    bool complete;
  };

  /**
   * Response to the StateRequest
   */
  struct StateResponse {
    /// A collection of keys-values states. Only populated if `no_proof` is
    /// `true`
    std::vector<KeyValueStateEntry> entries;
    /// If `no_proof` is false in request, this contains proof nodes.
    common::Buffer proof;
  };
}  // namespace kagome::network

#endif
