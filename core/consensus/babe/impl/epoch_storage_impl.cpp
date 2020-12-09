/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/epoch_storage_impl.hpp"

#include "primitives/digest.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

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
  const auto LAST_EPOCH_INDEX = common::Blob<8>::fromString("lstepind").value();

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

  bool EpochStorageImpl::exists(EpochIndex epoch_number) const {
    return storage_->contains(
        common::Buffer{EPOCH_PREFIX}.putUint64(epoch_number));
  }

  outcome::result<void> EpochStorageImpl::setLastEpoch(
      const LastEpochDescriptor &epoch_descriptor) {
    const auto &key = storage::kLastBabeEpochNumberLookupKey;
    auto val = common::Buffer{scale::encode(epoch_descriptor).value()};
    last_epoch_ = epoch_descriptor;
    return storage_->put(key, val);
  }

  outcome::result<LastEpochDescriptor> EpochStorageImpl::getLastEpoch() const {
    if (last_epoch_) {
      return last_epoch_.value();
    }
    const auto &key = storage::kLastBabeEpochNumberLookupKey;
    OUTCOME_TRY(epoch_descriptor, storage_->get(key));
    return scale::decode<LastEpochDescriptor>(epoch_descriptor);
  }
}  // namespace kagome::consensus
