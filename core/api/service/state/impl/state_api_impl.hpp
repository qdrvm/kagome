/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_API_IMPL_HPP
#define KAGOME_STATE_API_IMPL_HPP

#include "api/service/state/state_api.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "runtime/core.hpp"
#include "runtime/metadata.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::api {

  class StateApiImpl final : public StateApi {
   public:
    StateApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
                 std::shared_ptr<blockchain::BlockTree> block_tree,
                 std::shared_ptr<runtime::Core> runtime_core,
                 std::shared_ptr<runtime::Metadata> metadata);

    void setApiService(
        std::shared_ptr<api::ApiService> const &api_service) override;

    outcome::result<std::vector<common::Buffer>> getKeysPaged(
        const boost::optional<common::Buffer> &prefix,
        uint32_t keys_amount,
        const boost::optional<common::Buffer> &prev_key,
        const boost::optional<primitives::BlockHash> &block_hash_opt)
        const override;

    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) const override;
    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key,
        const primitives::BlockHash &at) const override;

    outcome::result<uint32_t> subscribeStorage(
        const std::vector<common::Buffer> &keys) override;
    outcome::result<bool> unsubscribeStorage(
        const std::vector<uint32_t> &subscription_id) override;

    outcome::result<primitives::Version> getRuntimeVersion(
        const boost::optional<primitives::BlockHash> &at) const override;

    outcome::result<uint32_t> subscribeRuntimeVersion() override;
    outcome::result<void> unsubscribeRuntimeVersion(
        uint32_t subscription_id) override;

    outcome::result<std::string> getMetadata() override;
    outcome::result<std::string> getMetadata(
        std::string_view hex_block_hash) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_repo_;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> runtime_core_;

    std::weak_ptr<api::ApiService> api_service_;
    std::shared_ptr<runtime::Metadata> metadata_;
  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_API_IMPL_HPP
