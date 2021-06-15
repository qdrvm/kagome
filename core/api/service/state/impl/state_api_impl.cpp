/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/impl/state_api_impl.hpp"
#include "common/hexutil.hpp"

#include <utility>

#include <jsonrpc-lean/fault.h>

namespace kagome::api {

  StateApiImpl::StateApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<runtime::Metadata> metadata)
      : block_repo_{std::move(block_repo)},
        storage_{std::move(trie_storage)},
        block_tree_{std::move(block_tree)},
        runtime_core_{std::move(runtime_core)},
        metadata_{std::move(metadata)} {
    BOOST_ASSERT(nullptr != block_repo_);
    BOOST_ASSERT(nullptr != storage_);
    BOOST_ASSERT(nullptr != block_tree_);
    BOOST_ASSERT(nullptr != runtime_core_);
    BOOST_ASSERT(nullptr != metadata_);
  }

  void StateApiImpl::setApiService(
      std::shared_ptr<api::ApiService> const &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<std::vector<common::Buffer>> StateApiImpl::getKeysPaged(
      const boost::optional<common::Buffer> &prefix_opt,
      uint32_t keys_amount,
      const boost::optional<common::Buffer> &prev_key_opt,
      const boost::optional<primitives::BlockHash> &block_hash_opt) const {
    const auto &prefix = prefix_opt.value_or(common::Buffer{});
    const auto &prev_key = prev_key_opt.value_or(prefix);
    const auto &block_hash =
        block_hash_opt.value_or(block_tree_->getLastFinalized().hash);

    OUTCOME_TRY(header, block_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(initial_trie_reader,
                storage_->getEphemeralBatchAt(header.state_root));
    auto cursor = initial_trie_reader->trieCursor();

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

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key) const {
    auto last_finalized = block_tree_->getLastFinalized();
    return getStorage(key, last_finalized.hash);
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key, const primitives::BlockHash &at) const {
    OUTCOME_TRY(header, block_repo_->getBlockHeader(at));
    OUTCOME_TRY(trie_reader, storage_->getEphemeralBatchAt(header.state_root));
    return trie_reader->get(key);
  }

  outcome::result<primitives::Version> StateApiImpl::getRuntimeVersion(
      const boost::optional<primitives::BlockHash> &at) const {
    if(at) {
      return runtime_core_->versionAt(at.value());
    }
    return runtime_core_->version();
  }

  outcome::result<uint32_t> StateApiImpl::subscribeStorage(
      const std::vector<common::Buffer> &keys) {
    if (auto api_service = api_service_.lock())
      return api_service->subscribeSessionToKeys(keys);

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<bool> StateApiImpl::unsubscribeStorage(
      const std::vector<uint32_t> &subscription_id) {
    if (auto api_service = api_service_.lock())
      return api_service->unsubscribeSessionFromIds(subscription_id);

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<uint32_t> StateApiImpl::subscribeRuntimeVersion() {
    if (auto api_service = api_service_.lock()) {
      return api_service->subscribeRuntimeVersion();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<void> StateApiImpl::unsubscribeRuntimeVersion(
      uint32_t subscription_id) {
    if (auto api_service = api_service_.lock()) {
      return api_service->unsubscribeRuntimeVersion(subscription_id)
          .as_failure();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<std::string> StateApiImpl::getMetadata() {
    OUTCOME_TRY(data, metadata_->metadata(boost::none));
    return common::hex_lower_0x(data);
  }

  outcome::result<std::string> StateApiImpl::getMetadata(
      std::string_view hex_block_hash) {
    OUTCOME_TRY(h, primitives::BlockHash::fromHexWithPrefix(hex_block_hash));
    OUTCOME_TRY(data, metadata_->metadata(h));
    return common::hex_lower_0x(data);
  }
}  // namespace kagome::api
