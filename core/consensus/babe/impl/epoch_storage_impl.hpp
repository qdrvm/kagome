/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_EPOCHSTORAGEIMPL
#define KAGOME_CONSENSUS_EPOCHSTORAGEIMPL

#include "consensus/babe/epoch_storage.hpp"

#include "blockchain/block_tree.hpp"
#include "primitives/babe_configuration.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus {

  enum class EpochStorageError { EPOCH_DOES_NOT_EXIST = 1 };

  /**
   * Implementation of epoch storage, which stores and returns epoch descriptors
   * based on their numbers. Epoch descriptors are stored on the disk
   */
  class EpochStorageImpl : public EpochStorage {
   public:
    ~EpochStorageImpl() override = default;

    explicit EpochStorageImpl(
        std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<kagome::storage::BufferStorage> storage);

    outcome::result<NextEpochDescriptor> getEpochDescriptor(
        BabeSlotNumber slot, primitives::BlockHash block_hash) const override;

    outcome::result<void> setLastEpoch(
        const LastEpochDescriptor &epoch_descriptor) override;

    outcome::result<LastEpochDescriptor> getLastEpoch() const override;

    bool contains(EpochIndex epoch_number) const override {
      assert(false);
    }

   private:
    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<kagome::storage::BufferStorage> storage_;
    boost::optional<LastEpochDescriptor>
        last_epoch_;  // optimization for storing in memory last epoch

    struct Node {
      primitives::BlockHash block_hash;

      std::set<Node> child;
    };

    std::unordered_map<primitives::BlockHash, std::shared_ptr<Node>> nodes_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, EpochStorageError);

#endif  // KAGOME_CONSENSUS_EPOCHSTORAGEIMPL
