/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHILD_STATE_API_IMPL_HPP
#define KAGOME_CHILD_STATE_API_IMPL_HPP

#include "api/service/child_state/child_state_api.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/metadata.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::api {

  class ChildStateApiImpl final : public ChildStateApi {
   public:
    ChildStateApiImpl(
        std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
        std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::Core> runtime_core,
        std::shared_ptr<runtime::Metadata> metadata);

    void setApiService(
        const std::shared_ptr<api::ApiService> &api_service) override;

    outcome::result<std::vector<common::Buffer>> getKeys(
        const common::Buffer &child_storage_key,
        const std::optional<common::Buffer> &prefix_opt,
        const std::optional<primitives::BlockHash> &block_hash_opt)
        const override;

    outcome::result<std::vector<common::Buffer>> getKeysPaged(
        const common::Buffer &child_storage_key,
        const std::optional<common::Buffer> &prefix_opt,
        uint32_t keys_amount,
        const std::optional<common::Buffer> &prev_key_opt,
        const std::optional<primitives::BlockHash> &block_hash_opt) const;

    outcome::result<std::optional<common::Buffer>> getStorage(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt)
        const override;

    outcome::result<std::optional<common::BlockHash>> getStorageHash(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt)
        const override;

    outcome::result<std::optional<uint64_t>> getStorageSize(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt)
        const override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> runtime_core_;

    std::weak_ptr<api::ApiService> api_service_;
    std::shared_ptr<runtime::Metadata> metadata_;
  };

}  // namespace kagome::api

#endif  // KAGOME_CHILD_STATE_API_IMPL_HPP
