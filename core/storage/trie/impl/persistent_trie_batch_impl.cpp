/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/persistent_trie_batch_impl.hpp"

#include <memory>

#include "storage/trie/impl/topper_trie_batch_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie,
                            PersistentTrieBatchImpl::Error,
                            e) {
  using E = kagome::storage::trie::PersistentTrieBatchImpl::Error;
  switch (e) {
    case E::NO_TRIE:
      return "Trie was not created or already was destructed.";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  PersistentTrieBatchImpl::PersistentTrieBatchImpl(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      TrieChangesTrackerOpt changes,
      std::shared_ptr<PolkadotTrie> trie,
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner)
      : TrieBatchBase{std::move(codec), std::move(serializer), std::move(trie)},
        changes_{std::move(changes)},
        state_pruner_{std::move(state_pruner)} {
    BOOST_ASSERT((changes_.has_value() && changes_.value() != nullptr)
                 or not changes_.has_value());
    BOOST_ASSERT(state_pruner_ != nullptr);
  }

  outcome::result<RootHash> PersistentTrieBatchImpl::commit(
      StateVersion version) {
    OUTCOME_TRY(commitChildren(version));
    OUTCOME_TRY(root, serializer_->storeTrie(*trie_, version));
    OUTCOME_TRY(state_pruner_->addNewState(*trie_, version));
    for (auto [child_key, child_trie] : getChildTries()) {
      OUTCOME_TRY(state_pruner_->addNewChildState(
          root, child_key, *child_trie, version));
    }
    SL_TRACE_FUNC_CALL(logger_, root);
    auto &stats = serializer_->getLatestStats();
    SL_DEBUG(logger_,
             "Written {} new nodes and {} values",
             stats.new_nodes_written,
             stats.values_written);
    return std::move(root);
  }

  outcome::result<std::tuple<bool, uint32_t>>
  PersistentTrieBatchImpl::clearPrefix(const BufferView &prefix,
                                       std::optional<uint64_t> limit) {
    SL_TRACE_VOID_FUNC_CALL(logger_, prefix);
    return trie_->clearPrefix(
        prefix, limit, [&](const auto &key, auto &&) -> outcome::result<void> {
          if (changes_.has_value()) {
            changes_.value()->onRemove(key);
          }
          return outcome::success();
        });
  }

  outcome::result<void> PersistentTrieBatchImpl::put(const BufferView &key,
                                                     BufferOrView &&value) {
    OUTCOME_TRY(contains, trie_->contains(key));
    bool is_new_entry = not contains;
    auto value_copy = value.mut();
    auto res = trie_->put(key, std::move(value));
    if (res and changes_.has_value()) {
      SL_TRACE_VOID_FUNC_CALL(logger_, key, value_copy);

      changes_.value()->onPut(key, value_copy, is_new_entry);
    }
    return res;
  }

  outcome::result<void> PersistentTrieBatchImpl::remove(const BufferView &key) {
    OUTCOME_TRY(trie_->remove(key));
    if (changes_.has_value()) {
      SL_TRACE_VOID_FUNC_CALL(logger_, key);
      changes_.value()->onRemove(key);
    }
    return outcome::success();
  }

  outcome::result<std::unique_ptr<TrieBatchBase>>
  PersistentTrieBatchImpl::createFromTrieHash(const RootHash &trie_hash) {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(trie_hash, nullptr));
    return std::make_unique<PersistentTrieBatchImpl>(
        codec_, serializer_, changes_, trie, state_pruner_);
  }

}  // namespace kagome::storage::trie
