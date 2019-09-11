/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/epoch_storage_dumb.hpp"

namespace kagome::consensus {
  EpochStorageDumb::EpochStorageDumb(std::shared_ptr<Babe> babe)
      : babe_{std::move(babe)} {}

  boost::optional<Epoch> EpochStorageDumb::getEpoch(
      const primitives::BlockId & /* ignored */) const {
    return babe_->getBabeMeta().current_epoch_;
  }
}  // namespace kagome::consensus
