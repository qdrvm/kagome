/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_JUSTIFICATION_STORAGE_POLICY_HPP
#define KAGOME_JUSTIFICATION_STORAGE_POLICY_HPP

#include "injector/lazy.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_header.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::blockchain {

  class JustificationStoragePolicy {
   public:
    virtual ~JustificationStoragePolicy() = default;

    virtual outcome::result<bool> shouldStoreFor(
        const primitives::BlockHeader &block,
        primitives::BlockNumber last_finalized_number) const = 0;
  };

  class JustificationStoragePolicyImpl final
      : public JustificationStoragePolicy {
   public:
    virtual outcome::result<bool> shouldStoreFor(
        const primitives::BlockHeader &block,
        primitives::BlockNumber last_finalized_number) const override;

    virtual void initBlockchainInfo();
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_JUSTIFICATION_STORAGE_POLICY_HPP
