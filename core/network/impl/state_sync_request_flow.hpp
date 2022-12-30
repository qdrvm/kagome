/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP
#define KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP

#include <unordered_map>
#include <unordered_set>

#include "network/types/state_request.hpp"
#include "network/types/state_response.hpp"
#include "primitives/block_header.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"

namespace kagome::runtime {
  class CoreApiFactory;
}  // namespace kagome::runtime

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::storage::trie {
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::primitives {
  struct BlockHeader;
}  // namespace kagome::primitives

namespace kagome::network {
  /**
   * https://github.com/paritytech/substrate/blob/master/client/network/sync/src/state.rs
   */
  class StateSyncRequestFlow {
    using RootHash = storage::trie::RootHash;
    struct Root {
      storage::trie::PolkadotTrieImpl trie;
      std::vector<common::Buffer> children;
    };

   public:
    enum class Error {
      EMPTY_RESPONSE = 1,
      HASH_MISMATCH,
    };

    StateSyncRequestFlow(const primitives::BlockInfo &block_info,
                         const primitives::BlockHeader &block);

    auto &blockInfo() const {
      return block_info_;
    }

    bool complete() const;

    StateRequest nextRequest() const;

    outcome::result<void> onResponse(const StateResponse &res);

    outcome::result<void> commit(
        const runtime::CoreApiFactory &core_api_factory,
        const std::shared_ptr<crypto::Hasher> &hasher,
        storage::trie::TrieSerializer &trie_serializer);

   private:
    primitives::BlockInfo block_info_;
    primitives::BlockHeader block_;

    std::vector<common::Buffer> last_key_;
    std::unordered_map<std::optional<RootHash>, Root> roots_;
    std::unordered_set<std::optional<RootHash>> complete_roots_;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, StateSyncRequestFlow::Error)

#endif  //  KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP
