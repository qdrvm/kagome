/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_API_IMPL_HPP
#define KAGOME_STATE_API_IMPL_HPP

#include "api/state/state_api.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "storage/trie/trie_db_backend.hpp"

namespace kagome::api {

  class StateApiImpl : public StateApi {
   public:
    StateApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<storage::trie::TrieDbBackend> trie_backend,
                 std::shared_ptr<blockchain::BlockTree> block_tree);

    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) override;
    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &at) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_repo_;
    std::shared_ptr<storage::trie::TrieDbBackend> trie_backend_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_API_IMPL_HPP
