/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/child_state/impl/child_state_api_impl.hpp"

#include <unordered_map>
#include <utility>

#include "common/hexutil.hpp"
#include "common/monadic_utils.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::api {

  ChildStateApiImpl::ChildStateApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<runtime::Metadata> metadata)
      : header_repo_{std::move(block_repo)},
        storage_{std::move(trie_storage)},
        block_tree_{std::move(block_tree)},
        runtime_core_{std::move(runtime_core)},
        metadata_{std::move(metadata)} {
    BOOST_ASSERT(nullptr != header_repo_);
    BOOST_ASSERT(nullptr != storage_);
    BOOST_ASSERT(nullptr != block_tree_);
    BOOST_ASSERT(nullptr != runtime_core_);
    BOOST_ASSERT(nullptr != metadata_);
  }

  void ChildStateApiImpl::setApiService(
      const std::shared_ptr<api::ApiService> &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<std::vector<common::Buffer>> ChildStateApiImpl::getKeys(
      const common::Buffer &child_storage_key,
      const std::optional<common::Buffer> &prefix_opt,
      const std::optional<primitives::BlockHash> &block_hash_opt) const {
    const auto &prefix = prefix_opt.value_or(common::emptyBuffer);
    const auto &block_hash =
        block_hash_opt.value_or(block_tree_->getLastFinalized().hash);

    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(initial_trie_reader,
                storage_->getEphemeralBatchAt(header.state_root));
    OUTCOME_TRY(child_root, initial_trie_reader->get(child_storage_key));
    OUTCOME_TRY(child_root_hash,
                common::Hash256::fromSpan(gsl::make_span(child_root.get())));
    OUTCOME_TRY(child_storage_trie_reader,
                storage_->getEphemeralBatchAt(child_root_hash));
    auto cursor = child_storage_trie_reader->trieCursor();

    OUTCOME_TRY(cursor->seekLowerBound(prefix));

    std::vector<common::Buffer> result{};
    while (cursor->isValid()) {
      auto key = cursor->key();
      BOOST_ASSERT(key.has_value());

      // make sure our key begins with prefix
      auto min_size = std::min(prefix.size(), key->size());
      if (not std::equal(
              prefix.begin(), prefix.begin() + min_size, key.value().begin())) {
        break;
      }
      result.push_back(cursor->key().value());
      OUTCOME_TRY(cursor->next());
    }

    return result;
  }

  outcome::result<std::vector<common::Buffer>> ChildStateApiImpl::getKeysPaged(
      const common::Buffer &child_storage_key,
      const std::optional<common::Buffer> &prefix_opt,
      uint32_t keys_amount,
      const std::optional<common::Buffer> &prev_key_opt,
      const std::optional<primitives::BlockHash> &block_hash_opt) const {
    const auto &prefix = prefix_opt.value_or(common::emptyBuffer);
    const auto &prev_key = prev_key_opt.value_or(prefix);
    const auto &block_hash =
        block_hash_opt.value_or(block_tree_->getLastFinalized().hash);

    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(initial_trie_reader,
                storage_->getEphemeralBatchAt(header.state_root));
    OUTCOME_TRY(child_root, initial_trie_reader->get(child_storage_key));
    OUTCOME_TRY(child_root_hash,
                common::Hash256::fromSpan(gsl::make_span(child_root.get())));
    OUTCOME_TRY(child_storage_trie_reader,
                storage_->getEphemeralBatchAt(child_root_hash));
    auto cursor = child_storage_trie_reader->trieCursor();

    // if prev_key is bigger than prefix, then set cursor to the next key after
    // prev_key
    if (prev_key > prefix) {
      OUTCOME_TRY(cursor->seekUpperBound(prev_key));
    }
    // otherwise set cursor to key that is next to or equal to prefix
    else {
      OUTCOME_TRY(cursor->seekLowerBound(prefix));
    }

    std::vector<common::Buffer> result{};
    result.reserve(keys_amount);
    for (uint32_t i = 0; i < keys_amount && cursor->isValid(); ++i) {
      auto key = cursor->key();
      BOOST_ASSERT(key.has_value());

      // make sure our key begins with prefix
      auto min_size = std::min(prefix.size(), key->size());
      if (not std::equal(
              prefix.begin(), prefix.begin() + min_size, key.value().begin())) {
        break;
      }
      result.push_back(cursor->key().value());
      OUTCOME_TRY(cursor->next());
    }

    return result;
  }

  outcome::result<std::optional<common::Buffer>> ChildStateApiImpl::getStorage(
      const common::Buffer &child_storage_key,
      const common::Buffer &key,
      const std::optional<primitives::BlockHash> &block_hash_opt) const {
    auto at = block_hash_opt ? block_hash_opt.value()
                             : block_tree_->getLastFinalized().hash;
    OUTCOME_TRY(header, header_repo_->getBlockHeader(at));
    OUTCOME_TRY(trie_reader, storage_->getEphemeralBatchAt(header.state_root));
    OUTCOME_TRY(child_root, trie_reader->get(child_storage_key));
    OUTCOME_TRY(child_root_hash,
                common::Hash256::fromSpan(gsl::make_span(child_root.get())));
    OUTCOME_TRY(child_storage_trie_reader,
                storage_->getEphemeralBatchAt(child_root_hash));
    auto res = child_storage_trie_reader->tryGet(key);
    return common::map_result_optional(res,
                                       [](const auto &r) { return r.get(); });
  }

  outcome::result<std::optional<primitives::BlockHash>>
  ChildStateApiImpl::getStorageHash(
      const common::Buffer &child_storage_key,
      const common::Buffer &key,
      const std::optional<primitives::BlockHash> &block_hash_opt) const {
    OUTCOME_TRY(value_opt, getStorage(child_storage_key, key, block_hash_opt));
    std::optional<primitives::BlockHash> hash_opt;
    if (value_opt.has_value()) {
      storage::trie::PolkadotCodec codec;
      auto hash =
          codec.hash256(common::Buffer(gsl::make_span(value_opt.value())));
      return hash;
    }
    return std::nullopt;
  }

  outcome::result<std::optional<uint64_t>> ChildStateApiImpl::getStorageSize(
      const common::Buffer &child_storage_key,
      const common::Buffer &key,
      const std::optional<primitives::BlockHash> &block_hash_opt) const {
    auto at = block_hash_opt ? block_hash_opt.value()
                             : block_tree_->getLastFinalized().hash;
    OUTCOME_TRY(header, header_repo_->getBlockHeader(at));
    OUTCOME_TRY(trie_reader, storage_->getEphemeralBatchAt(header.state_root));
    OUTCOME_TRY(child_root, trie_reader->get(child_storage_key));
    OUTCOME_TRY(child_root_hash,
                common::Hash256::fromSpan(gsl::make_span(child_root.get())));
    OUTCOME_TRY(child_storage_trie_reader,
                storage_->getEphemeralBatchAt(child_root_hash));
    OUTCOME_TRY(value, child_storage_trie_reader->get(key));
    return value.get().size();
  }
}  // namespace kagome::api
