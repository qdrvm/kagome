/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage {
  class RocksDbSpace;
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
    outcome::result<std::reference_wrapper<face::Readable<Buffer, Buffer>>>
    findLatestSource(BufferView key);

    std::shared_ptr<const DirectStorage> storage_;
    RootHash state_root_;
  };

  // TODO(Harrm) store large values separately?
  class DirectStorage final
      : public std::enable_shared_from_this<DirectStorage> {
   public:
    friend class DirectStorageView;

    static outcome::result<std::unique_ptr<DirectStorage>> create(
        std::shared_ptr<RocksDbSpace> direct_db,
        std::shared_ptr<RocksDbSpace> diff_db);

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
    struct ValueDeleted {};
    outcome::result<std::optional<std::variant<ValueDeleted, common::Buffer>>>
    getAt(const RootHash &state, common::BufferView key) const;
    outcome::result<std::optional<RootHash>> getStateParent(
        const RootHash &state) const;

    outcome::result<void> applyDiff(const RootHash &new_root);

    RootHash state_root_;
    std::shared_ptr<RocksDbSpace> direct_state_db_;
    std::shared_ptr<RocksDbSpace> diff_db_;
    log::Logger logger_ = log::createLogger("DirectStorage", "storage");
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, DirectStorageError)
