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

#include "consensus/babe/impl/epoch_storage_impl.hpp"

#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, EpochStorageError, e) {
  using E = kagome::consensus::EpochStorageError;

  switch (e) {
    case E::EPOCH_DOES_NOT_EXIST:
      return "Requested epoch does not exist";
  }
  return "Unknown error";
}

namespace kagome::consensus {

  const auto EPOCH_PREFIX = common::Blob<8>::fromString("epchdcr0").value();

  EpochStorageImpl::EpochStorageImpl(
      std::shared_ptr<kagome::storage::BufferStorage> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_);
  }

  outcome::result<void> EpochStorageImpl::addEpochDescriptor(
      EpochIndex epoch_number, const NextEpochDescriptor &epoch_descriptor) {
    auto key = common::Buffer{EPOCH_PREFIX}.putUint64(epoch_number);
    auto val = common::Buffer{scale::encode(epoch_descriptor).value()};
    return storage_->put(key, val);
  }

  outcome::result<NextEpochDescriptor> EpochStorageImpl::getEpochDescriptor(
      EpochIndex epoch_number) const {
    auto key = common::Buffer{EPOCH_PREFIX}.putUint64(epoch_number);
    OUTCOME_TRY(encoded_ed, storage_->get(key));
    return scale::decode<NextEpochDescriptor>(encoded_ed);
  }
}  // namespace kagome::consensus
