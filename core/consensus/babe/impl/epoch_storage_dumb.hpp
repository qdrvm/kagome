/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EPOCH_STORAGE_DUMB_HPP
#define KAGOME_EPOCH_STORAGE_DUMB_HPP

#include <memory>

#include "consensus/babe.hpp"
#include "consensus/babe/epoch_storage.hpp"

namespace kagome::consensus {
  /**
   * Dumb implementation of epoch storage, which always returns the current
   * epoch; later will be substituted by the one, which will use runtime API
   */
  class EpochStorageDumb : public EpochStorage {
   public:
    explicit EpochStorageDumb(std::shared_ptr<Babe> babe);

    ~EpochStorageDumb() override = default;

    boost::optional<Epoch> getEpoch(
        const primitives::BlockId &block_id) const override;

   private:
    std::shared_ptr<Babe> babe_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_EPOCH_STORAGE_DUMB_HPP
