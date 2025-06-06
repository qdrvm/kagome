/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/direct_storage.hpp"
#include "common/monadic_utils.hpp"
#include "consensus/timeline/timeline.hpp"
#include "primitives/block_header.hpp"
#include "storage/database_error.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, DirectStorageError, e) {
  switch (e) {
    case DirectStorageError::DISCONNECTED_UPDATE:
      return "Direct state updated to a state, that is not a descendant of the "
             "current direct state";

    case DirectStorageError::DISCONNECTED_DIFF:
      return "A state diff added that is not a descendant of a registered "
             "state";

    case DirectStorageError::DISCONNECTING_DISCARD:
      return "A state diff discarded that has non-discarded descendants";

    case DirectStorageError::ORPHANED_VIEW:
      return "Direct storage view references a state which diff is not present "
             "in the direct storage";
    case DirectStorageError::DIFF_TO_THIS_STATE_ALREADY_STORED:
      return "Diff with the same state root already stored. Attempt to store "
             "another one with the same root is suspicious";
    case DirectStorageError::DISCARD_UNKNOWN_DIFF:
      return "Discard requested of a diff that was not added to direct storage";
    case DirectStorageError::APPLY_UNKNOWN_DIFF:
      return "Apply requested for a diff that was not added to direct storage";
    case DirectStorageError::EMPTY_DIFF:
      return "Detected an empty diff. Diffs are not supposed to be empty";
  }
  __builtin_unreachable();
}

// TODO(Harrm): probably should cook up a normal function that returns an
// optional or a result instead
#define FIND_OR_RETURN(result, key, map, error)          \
  [[maybe_unused]] auto result##_it = (map).find((key)); \
  if (result##_it == (map).end()) {                      \
    return (error);                                      \
  }                                                      \
  [[maybe_unused]] auto &result = (result##_it)->second;

namespace kagome::storage::trie {

  const Buffer kLatestFinalizedStateKey =
      Buffer::fromString("kagome_latest_finalized_state");
  // the distance between the last finalized and last produced block is
  // typically pretty small, if we accumulate a lot of diffs it means that
  // something likely is wrong

  constexpr size_t kExpectedMaxDiffNum = 16;

  DirectStorageView::DirectStorageView(
      std::shared_ptr<const DirectStorage> storage, RootHash state_root)
      : storage_{std::move(storage)}, state_root_{state_root} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<BufferOrView> DirectStorageView::get(
      const BufferView &key) const {
    RootHash current_state = state_root_;
    for (;;) {
      if (storage_->state_root_ == current_state) {
        return storage_->direct_state_db_->get(key);
      }

      OUTCOME_TRY(diff_present, storage_->diff_db_->contains(current_state));
      if (!diff_present) {
        return DirectStorageError::ORPHANED_VIEW;
      }

      OUTCOME_TRY(value_opt, storage_->getAt(current_state, key));
      if (value_opt.has_value()) {
        auto &value = *value_opt;
        if (std::holds_alternative<Buffer>(value)) {
          return BufferOrView{std::move(std::get<Buffer>(value))};
        }
        return DatabaseError::NOT_FOUND;
      }
      OUTCOME_TRY(parent_opt, storage_->getStateParent(current_state));
      if (!parent_opt) {
        return DirectStorageError::ORPHANED_VIEW;
      }

      current_state = *parent_opt;
    }
  }

  outcome::result<std::optional<BufferOrView>> DirectStorageView::tryGet(
      const BufferView &key) const {
    auto res = get(key);
    if (res.has_error()) {
      if (res.error() == DatabaseError::NOT_FOUND) {
        return std::nullopt;
      }
      return res.error();
    }
    return std::optional{std::move(res.value())};
  }

  outcome::result<bool> DirectStorageView::contains(
      const BufferView &key) const {
    RootHash current_state = state_root_;

    for (;;) {
      if (storage_->state_root_ == current_state) {
        return storage_->direct_state_db_->contains(key);
      }
      OUTCOME_TRY(diff_present, storage_->diff_db_->contains(current_state));
      if (!diff_present) {
        return DirectStorageError::ORPHANED_VIEW;
      }

      OUTCOME_TRY(value_opt, storage_->getAt(current_state, key));
      if (value_opt.has_value()) {
        auto &value = *value_opt;
        return std::holds_alternative<Buffer>(value);
      }
      OUTCOME_TRY(parent_opt, storage_->getStateParent(current_state));
      if (!parent_opt) {
        return DirectStorageError::ORPHANED_VIEW;
      }
      current_state = *parent_opt;
    }
  }

  outcome::result<std::shared_ptr<DirectStorage>> DirectStorage::create(
      std::shared_ptr<BufferStorage> direct_db,
      std::shared_ptr<BufferStorage> diff_db,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      // primitives::events::SyncStateSubscriptionEnginePtr sync_sub_engine,
      LazySPtr<const consensus::Timeline> timeline) {
    std::shared_ptr<DirectStorage> storage{new DirectStorage{timeline}};
    storage->direct_state_db_ = direct_db;
    storage->diff_db_ = diff_db;
    storage->chain_event_sub_ =
        std::make_shared<primitives::events::ChainEventSubscriber>(
            std::move(chain_sub_engine));
    storage->chain_sub_id_ =
        storage->chain_event_sub_->generateSubscriptionSetId();
    storage->chain_event_sub_->subscribe(
        storage->chain_sub_id_,
        primitives::events::ChainEventType::kDiscardedHeads);
    storage->chain_event_sub_->subscribe(
        storage->chain_sub_id_,
        primitives::events::ChainEventType::kFinalizedHeads);
    storage->chain_event_sub_->subscribe(
        storage->chain_sub_id_,
        primitives::events::ChainEventType::kNewStateSynced);
    storage->chain_event_sub_->setCallback(
        [weak = storage->weak_from_this()](
            subscription::SubscriptionSetId id,
            auto,
            primitives::events::ChainEventType type,
            const primitives::events::ChainEventParams &params) {
          if (auto self = weak.lock()) {
            self->onChainEvent(id, nullptr, type, params);
          }
        });

    // storage->sync_event_sub_ =
    //     std::make_shared<primitives::events::SyncStateEventSubscriber>(
    //         std::move(sync_sub_engine));
    // storage->sync_sub_id_ =
    //     storage->sync_event_sub_->generateSubscriptionSetId();
    // storage->sync_event_sub_->subscribe(
    //     storage->sync_sub_id_,
    //     primitives::events::SyncStateEventType::kSyncState);
    // storage->sync_event_sub_->setCallback(
    //     [weak = storage->weak_from_this()](
    //         subscription::SubscriptionSetId id,
    //         auto,
    //         primitives::events::SyncStateEventType type,
    //         const primitives::events::SyncStateEventParams &params) {

    //     });

    OUTCOME_TRY(state_root, direct_db->tryGet(kLatestFinalizedStateKey));
    SL_DEBUG(storage->logger_,
             "Fetched last finalized state key: {}",
             common::map_optional(
                 common::map_optional(state_root, &BufferOrView::view),
                 &BufferView::toHex)
                 .value_or("<empty>"));
    BOOST_OUTCOME_TRY(
        storage->state_root_,
        RootHash::fromSpan(common::map_optional(state_root, &BufferOrView::view)
                               .value_or(storage::trie::kEmptyRootHash)));
    SL_VERBOSE(storage->logger_,
               "Initialize direct storage at state {}",
               common::map_optional(
                   common::map_optional(state_root, &BufferOrView::view),
                   &BufferView::toHex)
                   .value_or("<empty>"));
    return storage;
  }

  DirectStorage::DirectStorage(LazySPtr<const consensus::Timeline> timeline)
      : timeline_(timeline) {}

  void DirectStorage::onChainEvent(
      subscription::SubscriptionSetId id,
      void *,
      primitives::events::ChainEventType type,
      const primitives::events::ChainEventParams &params) {
    BOOST_ASSERT(id == chain_sub_id_);
    if (type == primitives::events::ChainEventType::kDiscardedHeads) {
      const primitives::BlockHeader &header =
          std::get<primitives::events::HeadsEventParams>(params).get();
      auto res = discardDiff(header.state_root);
      if (!res) {
        SL_ERROR(logger_,
                 "Failed to discard diff for block {}, state root {}: {}",
                 header.blockInfo(),
                 header.state_root,
                 res.error());
      }
    } else if (type == primitives::events::ChainEventType::kFinalizedHeads) {
      const primitives::BlockHeader &header =
          std::get<primitives::events::HeadsEventParams>(params).get();
      auto res = updateDirectState(header.state_root);
      if (!res) {
        SL_ERROR(logger_,
                 "Failed to set direct state at block {}, state root {}: {}",
                 header.blockInfo(),
                 header.state_root,
                 res.error());
      }
    } else if (type == primitives::events::ChainEventType::kNewStateSynced) {
      const auto &[root, trie] =
          std::get<primitives::events::NewStateSyncedParams>(params);
      auto res = resetDirectState(root, trie);
      if (!res) {
        SL_ERROR(logger_,
                 "Failed to reset direct state after state sync at root {}: {}",
                 root,
                 res.error());
      }
    }
  }

  const RootHash &DirectStorage::getDirectStateRoot() const {
    return state_root_;
  }

  outcome::result<void> DirectStorage::resetDirectState(
      const RootHash &new_state_root, const PolkadotTrie &new_state) {
    SL_VERBOSE(logger_,
               "Start resetting direct storage to new state ",
               new_state_root);
    OUTCOME_TRY(diff_db_->clear());
    OUTCOME_TRY(direct_state_db_->clear());

    auto batch = direct_state_db_->batch();
    size_t count{};
    auto checkpoint = std::chrono::steady_clock::now();
    auto cursor = new_state.trieCursor();
    OUTCOME_TRY(cursor->seekFirst());
    BOOST_ASSERT(cursor->isValid());
    while (cursor->isValid()) {
      OUTCOME_TRY(batch->put(cursor->key().value(), cursor->value().value()));
      OUTCOME_TRY(cursor->next());
      ++count;
      auto now = std::chrono::steady_clock::now();
      if (now - checkpoint > std::chrono::seconds(1)) {
        SL_DEBUG(logger_,
                 "Inserted {} keys into direct storage with root {}",
                 count,
                 new_state_root);
        checkpoint = now;
      }
    }
    SL_VERBOSE(logger_,
               "Inserted total of {} keys into direct storage with root {}",
               count,
               new_state_root);
    OUTCOME_TRY(batch->put(kLatestFinalizedStateKey, new_state_root));
    OUTCOME_TRY(batch->commit());
    SL_DEBUG(logger_, "Put kLatestFinalizedStateKey {}", new_state_root);
    state_root_ = new_state_root;
    return outcome::success();
  }

  outcome::result<void> DirectStorage::updateDirectState(
      const RootHash &target_state) {
    std::vector<RootHash> diffs;
    RootHash current_state = target_state;
    size_t idx = 0;
    while (current_state != state_root_) {
      OUTCOME_TRY(diff_known, diff_db_->contains(current_state));
      if (!diff_known) {
        return DirectStorageError::DISCONNECTED_UPDATE;
      }

      diffs.emplace_back(current_state);
      OUTCOME_TRY(parent_opt, getStateParent(current_state));
      if (!parent_opt) {
        return DirectStorageError::DISCONNECTED_UPDATE;
      }
      // TOOD(Harrm) should probably be trace
      SL_DEBUG(logger_,
               "#{}, Accumulating diffs for update, {} -> {}",
               idx++,
               *parent_opt,
               current_state);

      current_state = *parent_opt;
    }

    for (auto root : diffs | std::views::reverse) {
      OUTCOME_TRY(applyDiff(root));
      OUTCOME_TRY(discardDiff(root));
      state_root_ = root;
    }

    // TODO(Harrm): maybe cleanup now orphaned diffs, although they should be
    // discarded with their blocks anyway
    SL_VERBOSE(logger_, "Update direct storage to new state {}", target_state);

    return outcome::success();
  }

  outcome::result<void> DirectStorage::storeDiff(DiffRoots roots,
                                                 StateDiff &&diff) {
    if (diff.empty()) {
      return DirectStorageError::EMPTY_DIFF;
    }
    OUTCOME_TRY(from_diff_known, diff_db_->contains(roots.from));
    if (roots.from != state_root_ && !from_diff_known) {
      return DirectStorageError::DISCONNECTED_DIFF;
    }
    OUTCOME_TRY(to_diff_known, diff_db_->contains(roots.to));
    if (to_diff_known) {
      return DirectStorageError::DIFF_TO_THIS_STATE_ALREADY_STORED;
    }

    auto diff_batch = diff_db_->batch();
    for (auto &[key, val] : diff) {
      Buffer full_key;
      full_key.put(roots.to);
      full_key.put(key);
      if (val.has_value()) {
        val->putUint8(1);
        OUTCOME_TRY(diff_batch->put(full_key, std::move(*val)));

      } else {
        OUTCOME_TRY(diff_batch->put(full_key, Buffer{0}));
      }
    }
    // remember parent state
    OUTCOME_TRY(diff_batch->put(roots.to, roots.from));
    OUTCOME_TRY(diff_batch->commit());

    SL_DEBUG(logger_,
             "Store new diff for state transition from {} to {}",
             roots.from,
             roots.to);
    if (!timeline_.get()->wasSynchronized()) {
      OUTCOME_TRY(updateDirectState(roots.to));
      SL_DEBUG(
          logger_,
          "Since node the node is not synchronized, update to this state.");
    }
    return outcome::success();
  }

  outcome::result<void> DirectStorage::discardDiff(const RootHash &to_state) {
    OUTCOME_TRY(diff_known, diff_db_->contains(to_state));
    if (!diff_known) {
      return DirectStorageError::DISCARD_UNKNOWN_DIFF;
    }
    OUTCOME_TRY(parent_opt, getStateParent(to_state));
    if (!parent_opt) {
      return DirectStorageError::DISCARD_UNKNOWN_DIFF;
    }

    OUTCOME_TRY(diff_db_->remove(to_state));
    SL_WARN(logger_, "TODO(Harrm): IMPORTANT remove all diff entries");

    SL_DEBUG(logger_,
             "Discard diff for state transition from {} to {}",
             *parent_opt,
             to_state);
    return outcome::success();
  }

  outcome::result<std::unique_ptr<DirectStorageView>> DirectStorage::getViewAt(
      const RootHash &state_root) const {
    OUTCOME_TRY(diff_known, diff_db_->contains(state_root));

    if (state_root != state_root_ && !diff_known) {
      SL_DEBUG(logger_,
               "Failed to get direct storage view at state {}: no such state "
               "stored in direct storage",
               state_root);
      return DirectStorageError::ORPHANED_VIEW;
    }
    SL_DEBUG(logger_, "Get direct storage view at state {}", state_root);
    return std::make_unique<DirectStorageView>(shared_from_this(), state_root);
  }

  outcome::result<
      std::optional<std::variant<DirectStorage::ValueDeleted, common::Buffer>>>
  DirectStorage::getAt(const RootHash &state, common::BufferView key) const {
    Buffer full_key;
    full_key.put(state);
    full_key.put(key);
    OUTCOME_TRY(value_opt, diff_db_->tryGet(full_key));
    if (!value_opt) {
      return std::nullopt;
    }
    auto value = value_opt->intoBuffer();
    if (value[value.size() - 1] == 1) {
      value.pop_back();
      return std::variant<ValueDeleted, common::Buffer>(std::move(value));
    }
    return std::variant<ValueDeleted, common::Buffer>(ValueDeleted{});
  }

  outcome::result<std::optional<RootHash>> DirectStorage::getStateParent(
      const RootHash &state) const {
    OUTCOME_TRY(parent_opt, diff_db_->tryGet(state));
    if (parent_opt) {
      OUTCOME_TRY(root, RootHash::fromSpan(parent_opt->view()));
      return root;
    }
    return std::nullopt;
  }

  outcome::result<void> DirectStorage::applyDiff(const RootHash &new_root) {
    SL_DEBUG(logger_, "Start applying diff to state {}", new_root);

    OUTCOME_TRY(diff_known, diff_db_->contains(new_root));
    if (!diff_known) {
      SL_DEBUG(
          logger_, "Apply failed, diff for state {} was not stored", new_root);
      return DirectStorageError::APPLY_UNKNOWN_DIFF;
    }

    MapPrefix diff_prefix{new_root, diff_db_};

    auto batch = direct_state_db_->batch();
    size_t num = 0;
    auto iter = diff_prefix.cursor();
    OUTCOME_TRY(iter->seekFirst());
    if (!iter->isValid()) {
      return DirectStorageError::EMPTY_DIFF;
    }
    while (iter->isValid()) {
      auto key = *iter->key();
      auto value = *iter->value();
      if (value.view() == std::array<uint8_t, 1>{0}) {
        OUTCOME_TRY(batch->remove(key));
      } else {
        auto value_buf = std::move(value).intoBuffer();
        value_buf.pop_back();
        OUTCOME_TRY(batch->put(key, std::move(value_buf)));
      }
      ++num;
      OUTCOME_TRY(iter->next());
    }

    OUTCOME_TRY(batch->put(kLatestFinalizedStateKey, new_root));
    SL_DEBUG(logger_, "Put kLatestFinalizedStateKey {}", new_root);

    OUTCOME_TRY(batch->commit());
    SL_DEBUG(logger_, "Applied diff to state {} with {} writes", new_root, num);

    return outcome::success();
  }

}  // namespace kagome::storage::trie
