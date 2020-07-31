/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_API_IMPL_HPP
#define KAGOME_STATE_API_IMPL_HPP

#include "api/service/api_service.hpp"
#include "api/service/state/readonly_trie_builder.hpp"
#include "api/service/state/state_api.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "runtime/core.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::api {

  class StateApiImpl final : public StateApi {
   public:
    StateApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
                 std::shared_ptr<blockchain::BlockTree> block_tree,
                 std::shared_ptr<runtime::Core> runtime_core,
                 std::shared_ptr<api::ApiService> api_service);

    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) const override;
    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key,
        const primitives::BlockHash &at) const override;
    outcome::result<primitives::Version> getRuntimeVersion(
        const boost::optional<primitives::BlockHash> &at) const override;
    outcome::result<uint32_t> subscribeStorage(
        const std::vector<common::Buffer> &keys) override;
    outcome::result<void> unsubscribeStorage(
        uint32_t subscription_id) override;

  private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_repo_;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> runtime_core_;
    std::shared_ptr<api::ApiService> api_service_;
  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_API_IMPL_HPP
