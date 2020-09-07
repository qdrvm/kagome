/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EPOCH_STORAGE_DUMB_HPP
#define KAGOME_EPOCH_STORAGE_DUMB_HPP

#include "consensus/babe/epoch_storage.hpp"

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

    explicit EpochStorageImpl(std::shared_ptr<storage::BufferStorage> storage);

    outcome::result<void> addEpochDescriptor(
        EpochIndex epoch_number,
        const NextEpochDescriptor &epoch_descriptor) override;

    outcome::result<NextEpochDescriptor> getEpochDescriptor(
        EpochIndex epoch_number) const override;

    outcome::result<void> setLastEpoch(
        const LastEpochDescriptor &epoch_descriptor) override;

    outcome::result<LastEpochDescriptor> getLastEpoch() const override;

   private:
    std::shared_ptr<storage::BufferStorage> storage_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, EpochStorageError);

#endif  // KAGOME_EPOCH_STORAGE_DUMB_HPP
