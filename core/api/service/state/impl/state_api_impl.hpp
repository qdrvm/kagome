/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/state/state_api.hpp"

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "injector/lazy.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/metadata.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {
  class Executor;
}

namespace kagome::api {

  class StateApiImpl final : public StateApi {
   public:
    enum class Error {
      MAX_BLOCK_RANGE_EXCEEDED = 1,
      MAX_KEY_SET_SIZE_EXCEEDED,
      END_BLOCK_LOWER_THAN_BEGIN_BLOCK
    };

    static constexpr size_t kMaxBlockRange = 256;
    static constexpr size_t kMaxKeySetSize = 64;

    StateApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
                 std::shared_ptr<blockchain::BlockTree> block_tree,
                 std::shared_ptr<runtime::Core> runtime_core,
                 std::shared_ptr<runtime::Metadata> metadata,
                 std::shared_ptr<runtime::Executor> executor,
                 LazySPtr<api::ApiService> api_service);

    outcome::result<common::Buffer> call(
        std::string_view method,
        common::Buffer data,
        const std::optional<primitives::BlockHash> &opt_at) const override;

    outcome::result<std::vector<common::Buffer>> getKeysPaged(
        const std::optional<common::BufferView> &prefix,
        uint32_t keys_amount,
        const std::optional<common::BufferView> &prev_key,
        const std::optional<primitives::BlockHash> &block_hash_opt)
        const override;

    outcome::result<std::optional<common::Buffer>> getStorage(
        common::BufferView key) const override;

    outcome::result<std::optional<common::Buffer>> getStorageAt(
        common::BufferView key, const primitives::BlockHash &at) const override;

    outcome::result<std::optional<uint64_t>> getStorageSize(
        common::BufferView key,
        const std::optional<primitives::BlockHash> &block_hash_opt)
        const override;

    outcome::result<std::vector<StorageChangeSet>> queryStorage(
        std::span<const common::Buffer> keys,
        const primitives::BlockHash &from,
        std::optional<primitives::BlockHash> to) const override;

    outcome::result<std::vector<StorageChangeSet>> queryStorageAt(
        std::span<const common::Buffer> keys,
        std::optional<primitives::BlockHash> at) const override;

    outcome::result<ReadProof> getReadProof(
        std::span<const common::Buffer> keys,
        std::optional<primitives::BlockHash> at) const override;

    outcome::result<uint32_t> subscribeStorage(
        const std::vector<common::Buffer> &keys) override;

    outcome::result<bool> unsubscribeStorage(
        const std::vector<uint32_t> &subscription_id) override;

    outcome::result<primitives::Version> getRuntimeVersion(
        const std::optional<primitives::BlockHash> &at) const override;

    outcome::result<uint32_t> subscribeRuntimeVersion() override;
    outcome::result<void> unsubscribeRuntimeVersion(
        uint32_t subscription_id) override;

    outcome::result<std::string> getMetadata() override;
    outcome::result<std::string> getMetadata(
        std::string_view hex_block_hash) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> runtime_core_;

    LazySPtr<api::ApiService> api_service_;
    std::shared_ptr<runtime::Metadata> metadata_;
    std::shared_ptr<runtime::Executor> executor_;
  };

}  // namespace kagome::api

OUTCOME_HPP_DECLARE_ERROR(kagome::api, StateApiImpl::Error);
