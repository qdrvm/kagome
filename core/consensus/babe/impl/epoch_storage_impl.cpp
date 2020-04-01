/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/epoch_storage_impl.hpp"

#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, EpochStorageError, e) {
  using E = kagome::consensus::EpochStorageError;

  switch (e) {
    case E::EPOCH_DOES_NOT_EXIST:
      return "Requested epoch does not exist";
  }
}

namespace kagome::consensus {

  const static auto EPOCH_PREFIX =
      common::Blob<8>::fromString("epchdcr0").value();

  EpochStorageImpl::EpochStorageImpl(
      std::shared_ptr<kagome::storage::BufferStorage> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_);
  }

  void EpochStorageImpl::addEpochDescriptor(
      EpochIndex epoch_number, NextEpochDescriptor epoch_descriptor) {
    auto key = common::Buffer{EPOCH_PREFIX}.putUint64(epoch_number);
    auto val = common::Buffer{scale::encode(epoch_descriptor).value()};
    auto put_res = storage_->put(key, val);
    BOOST_ASSERT(put_res);
  }

  outcome::result<NextEpochDescriptor> EpochStorageImpl::getEpochDescriptor(
      EpochIndex epoch_number) const {
    auto key = common::Buffer{EPOCH_PREFIX}.putUint64(epoch_number);
    OUTCOME_TRY(encoded_ed, storage_->get(key));
    return scale::decode<NextEpochDescriptor>(encoded_ed);
  }
}  // namespace kagome::consensus
