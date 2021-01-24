/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/epoch_storage_impl.hpp"

#include "babe_digests_util.hpp"
#include "consensus/babe/types/last_epoch_descriptor.hpp"
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

  EpochStorageImpl::EpochStorageImpl(
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<kagome::storage::BufferStorage> storage)
      : babe_configuration_{std::move(babe_configuration)},
        storage_{std::move(storage)} {
    BOOST_ASSERT(babe_configuration_);
    BOOST_ASSERT(storage_);
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
