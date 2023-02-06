/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/impl/state_api_impl.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <unordered_map>
#include <utility>

#include <jsonrpc-lean/fault.h>

#include "common/hexutil.hpp"
#include "common/monadic_utils.hpp"
#include "common/no_cb.hpp"
#include "runtime/common/executor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, StateApiImpl::Error, e) {
  using E = kagome::api::StateApiImpl::Error;
  switch (e) {
    case E::MAX_BLOCK_RANGE_EXCEEDED:
      return "Maximum block range size ("
             + std::to_string(kagome::api::StateApiImpl::kMaxBlockRange)
             + " blocks) exceeded";
    case E::MAX_KEY_SET_SIZE_EXCEEDED:
      return "Maximum key set size ("
             + std::to_string(kagome::api::StateApiImpl::kMaxKeySetSize)
             + " keys) exceeded";
    case E::END_BLOCK_LOWER_THAN_BEGIN_BLOCK:
      return "End block is lower (is an ancestor of) the begin block "
             "(should be the other way)";
  }
  return "Unknown State API error";
}

namespace kagome::api {

  StateApiImpl::StateApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<runtime::Metadata> metadata,
      std::shared_ptr<runtime::RawExecutor> executor)
      : header_repo_{std::move(block_repo)},
        storage_{std::move(trie_storage)},
        block_tree_{std::move(block_tree)},
        runtime_core_{std::move(runtime_core)},
        metadata_{std::move(metadata)},
        executor_{std::move(executor)} {
    BOOST_ASSERT(nullptr != header_repo_);
    BOOST_ASSERT(nullptr != storage_);
    BOOST_ASSERT(nullptr != block_tree_);
    BOOST_ASSERT(nullptr != runtime_core_);
    BOOST_ASSERT(nullptr != metadata_);
    BOOST_ASSERT(nullptr != executor_);
  }

  void StateApiImpl::setApiService(
      std::shared_ptr<api::ApiService> const &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<common::Buffer> StateApiImpl::call(
      std::string_view method,
      common::Buffer data,
      const std::optional<primitives::BlockHash> &opt_at) const {
    auto at =
        opt_at.has_value() ? opt_at.value() : block_tree_->bestLeaf().hash;
    return executor_->callAtRaw(at, method, data, kNoCb);
  }

  outcome::result<std::vector<common::Buffer>> StateApiImpl::getKeysPaged(
      const std::optional<common::BufferView> &prefix_opt,
      uint32_t keys_amount,
      const std::optional<common::BufferView> &prev_key_opt,
      const std::optional<primitives::BlockHash> &block_hash_opt) const {
    const auto &prefix = prefix_opt.value_or(common::kEmptyBuffer);
    const auto &prev_key = prev_key_opt.value_or(prefix);
    const auto &block_hash =
        block_hash_opt.value_or(block_tree_->getLastFinalized().hash);

    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(initial_trie_reader,
                storage_->getEphemeralBatchAt(header.state_root, kNoCb));
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
      if (!boost::starts_with(key.value(), prefix)) {
        break;
      }
      result.push_back(cursor->key().value());
      OUTCOME_TRY(cursor->next());
    }

    return result;
  }

  outcome::result<std::optional<common::Buffer>> StateApiImpl::getStorage(
      const common::BufferView &key) const {
    auto last_finalized = block_tree_->getLastFinalized();
    return getStorageAt(key, last_finalized.hash);
  }

  outcome::result<std::optional<common::Buffer>> StateApiImpl::getStorageAt(
      const common::BufferView &key, const primitives::BlockHash &at) const {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(at));
    OUTCOME_TRY(trie_reader,
                storage_->getEphemeralBatchAt(header.state_root, kNoCb));
    auto res = trie_reader->tryGet(key);
    return common::map_result_optional(
        std::move(res), [](common::BufferOrView &&r) { return r.into(); });
  }

  outcome::result<std::vector<StateApiImpl::StorageChangeSet>>
  StateApiImpl::queryStorage(
      gsl::span<const common::Buffer> keys,
      const primitives::BlockHash &from,
      std::optional<primitives::BlockHash> opt_to) const {
    // TODO(Harrm): Optimize once changes trie is enabled (and a warning/assert
    // for now that will fire once it is, just not to forget)
    auto to =
        opt_to.has_value() ? opt_to.value() : block_tree_->bestLeaf().hash;
    if (keys.size() > static_cast<ssize_t>(kMaxKeySetSize)) {
      return Error::MAX_KEY_SET_SIZE_EXCEEDED;
    }

    if (from != to) {
      OUTCOME_TRY(from_number, header_repo_->getNumberByHash(from));
      OUTCOME_TRY(to_number, header_repo_->getNumberByHash(to));
      if (to_number < from_number) {
        return Error::END_BLOCK_LOWER_THAN_BEGIN_BLOCK;
      }
      if (to_number - from_number > kMaxBlockRange) {
        return Error::MAX_BLOCK_RANGE_EXCEEDED;
      }
    }

    std::vector<StorageChangeSet> changes;
    std::map<gsl::span<const uint8_t>, std::optional<common::Buffer>>
        last_values;

    // TODO(Harrm): optimize it to use a lazy generator instead of returning the
    // whole vector with block ids
    OUTCOME_TRY(range, block_tree_->getChainByBlocks(from, to));
    for (auto &block : range) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block));
      OUTCOME_TRY(batch,
                  storage_->getEphemeralBatchAt(header.state_root, kNoCb));
      StorageChangeSet change{block, {}};
      for (auto &key : keys) {
        OUTCOME_TRY(opt_get, batch->tryGet(key));
        auto opt_value = common::map_optional(
            std::move(opt_get),
            [](common::BufferOrView &&r) { return r.into(); });
        auto it = last_values.find(key);
        if (it == last_values.end() || it->second != opt_value) {
          change.changes.push_back(StorageChangeSet::Change{key, opt_value});
        }
        last_values[key] = std::move(opt_value);
      }
      if (!change.changes.empty()) {
        changes.emplace_back(std::move(change));
      }
    }
    return changes;
  }

  outcome::result<std::vector<StateApiImpl::StorageChangeSet>>
  StateApiImpl::queryStorageAt(
      gsl::span<const common::Buffer> keys,
      std::optional<primitives::BlockHash> opt_at) const {
    auto at =
        opt_at.has_value() ? opt_at.value() : block_tree_->bestLeaf().hash;
    return queryStorage(keys, at, at);
  }

  outcome::result<primitives::Version> StateApiImpl::getRuntimeVersion(
      const std::optional<primitives::BlockHash> &at) const {
    if (at) {
      return runtime_core_->version(at.value());
    }
    return runtime_core_->version(block_tree_->bestLeaf().hash);
  }

  outcome::result<uint32_t> StateApiImpl::subscribeStorage(
      const std::vector<common::Buffer> &keys) {
    if (auto api_service = api_service_.lock()) {
      return api_service->subscribeSessionToKeys(keys);
    }

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
      OUTCOME_TRY(api_service->unsubscribeRuntimeVersion(subscription_id));
      return outcome::success();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<std::string> StateApiImpl::getMetadata() {
    OUTCOME_TRY(data, metadata_->metadata(block_tree_->bestLeaf().hash));
    return common::hex_lower_0x(data);
  }

  outcome::result<std::string> StateApiImpl::getMetadata(
      std::string_view hex_block_hash) {
    OUTCOME_TRY(h, primitives::BlockHash::fromHexWithPrefix(hex_block_hash));
    OUTCOME_TRY(data, metadata_->metadata(h));
    return common::hex_lower_0x(data);
  }
}  // namespace kagome::api
