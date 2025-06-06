/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "common/buffer.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage {
  class RocksDbSpace;
}

namespace kagome::consensus {
  class Timeline;
}

namespace kagome::storage::trie {

  class PolkadotTrie;

  enum class DirectStorageError : uint8_t {
    DISCONNECTED_UPDATE = 1,
    DISCONNECTED_DIFF,
    DISCONNECTING_DISCARD,
    DISCARD_UNKNOWN_DIFF,
    ORPHANED_VIEW,
    DIFF_TO_THIS_STATE_ALREADY_STORED,
    APPLY_UNKNOWN_DIFF,
    EMPTY_DIFF,
  };

  using StateDiff = std::unordered_map<Buffer, std::optional<Buffer>>;

  class DirectStorage;

  class DirectStorageView final : public face::Readable<Buffer, Buffer> {
   public:
    DirectStorageView(std::shared_ptr<const DirectStorage> storage,
                      RootHash state_root);

    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;

    outcome::result<bool> contains(const BufferView &key) const override;

    const RootHash &getStateRoot() const {
      return state_root_;
    }

   private:
    std::shared_ptr<const DirectStorage> storage_;
    RootHash state_root_;
  };

  // TODO(Harrm) store large values separately?
  class DirectStorage final
      : public std::enable_shared_from_this<DirectStorage> {
   public:
    friend class DirectStorageView;

    static outcome::result<std::shared_ptr<DirectStorage>> create(
        std::shared_ptr<BufferStorage> direct_db,
        std::shared_ptr<BufferStorage> diff_db,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        LazySPtr<const consensus::Timeline> timeline);

    const RootHash &getDirectStateRoot() const;

    outcome::result<void> resetDirectState(const RootHash &new_state_root,
                                           const PolkadotTrie &new_state);

    outcome::result<void> updateDirectState(const RootHash &target_state);

    struct DiffRoots {
      const RootHash &from;
      const RootHash &to;
    };
    outcome::result<void> storeDiff(DiffRoots roots, StateDiff &&diff);
    outcome::result<void> discardDiff(const RootHash &to_state);

    outcome::result<std::unique_ptr<DirectStorageView>> getViewAt(
        const RootHash &state_root) const;

   private:
    explicit DirectStorage(LazySPtr<const consensus::Timeline> timeline);

    void onChainEvent(subscription::SubscriptionSetId id,
                      void *,
                      primitives::events::ChainEventType type,
                      const primitives::events::ChainEventParams &params);

    struct ValueDeleted {};
    outcome::result<std::optional<std::variant<ValueDeleted, common::Buffer>>>
    getAt(const RootHash &state, common::BufferView key) const;
    outcome::result<std::optional<RootHash>> getStateParent(
        const RootHash &state) const;

    outcome::result<void> applyDiff(const RootHash &new_root);

    RootHash state_root_;
    std::shared_ptr<BufferStorage> direct_state_db_;
    std::shared_ptr<BufferStorage> diff_db_;
    subscription::SubscriptionSetId chain_sub_id_ {};
    //subscription::SubscriptionSetId sync_sub_id_;
    primitives::events::ChainEventSubscriberPtr chain_event_sub_;
    //primitives::events::SyncStateEventSubscriberPtr sync_event_sub_;
    LazySPtr<const consensus::Timeline> timeline_;

    log::Logger logger_ = log::createLogger("DirectStorage", "storage");
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, DirectStorageError)
