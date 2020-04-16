/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_EPOCH_STORAGE_DUMB_HPP
#define KAGOME_EPOCH_STORAGE_DUMB_HPP

#include "consensus/babe/epoch_storage.hpp"

#include <unordered_map>
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

   private:
    std::shared_ptr<storage::BufferStorage> storage_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, EpochStorageError);

#endif  // KAGOME_EPOCH_STORAGE_DUMB_HPP
