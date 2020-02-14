/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_API_IMPL_HPP
#define KAGOME_STATE_API_IMPL_HPP

#include <mutex>

#include "api/state/state_api.hpp"
#include "blockchain/block_header_repository.hpp"
#include "storage/trie/trie_db_reader.hpp"

namespace kagome::api {

  class StateApiImpl : public StateApi {
   public:
    StateApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<storage::trie::TrieDbReader> trie_db);

    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) override;
    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &at) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_repo_;
    std::shared_ptr<storage::trie::TrieDbReader> trie_db_;
    std::mutex storage_mutex_;
  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_API_IMPL_HPP
