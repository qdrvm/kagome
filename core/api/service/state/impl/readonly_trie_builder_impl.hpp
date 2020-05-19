/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_READONLY_TRIE_BUILDER_IMPL_HPP
#define KAGOME_API_STATE_READONLY_TRIE_BUILDER_IMPL_HPP

#include "api/service/state/readonly_trie_builder.hpp"
#include "storage/trie/trie_db_backend.hpp"

namespace kagome::api {

  class ReadonlyTrieBuilderImpl : public ReadonlyTrieBuilder {
   public:
    explicit ReadonlyTrieBuilderImpl(
        std::shared_ptr<storage::trie::TrieDbBackend> backend);
    ~ReadonlyTrieBuilderImpl() override = default;

    std::unique_ptr<storage::trie::TrieDbReader> buildAt(
        primitives::BlockHash state_root) const override;

   private:
    std::shared_ptr<storage::trie::TrieDbBackend> backend_;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_READONLY_TRIE_BUILDER_IMPL_HPP
