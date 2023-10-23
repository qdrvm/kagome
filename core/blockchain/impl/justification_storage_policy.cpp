/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/justification_storage_policy.hpp"

#include <memory>

#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"

namespace kagome::blockchain {

  outcome::result<bool> JustificationStoragePolicyImpl::shouldStoreFor(
      const primitives::BlockHeader &block_header,
      primitives::BlockNumber last_finalized_number) const {
    if (block_header.number == 0) {
      return true;
    }

    BOOST_ASSERT_MSG(last_finalized_number >= block_header.number,
                     "Target block must be finalized");

    if (consensus::grandpa::HasAuthoritySetChange{block_header}) {
      return true;
    }
    if (block_header.number % 512 == 0) {
      return true;
    }
    return false;
  }

  void JustificationStoragePolicyImpl::initBlockchainInfo() {}

}  // namespace kagome::blockchain
