/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/justification_storage_policy.hpp"

#include <memory>

#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"

namespace kagome::blockchain {

  JustificationStoragePolicyImpl::JustificationStoragePolicyImpl(
      lazy<std::shared_ptr<const blockchain::BlockTree>> block_tree)
      : block_tree_(std::move(block_tree)) {}

  outcome::result<bool> JustificationStoragePolicyImpl::shouldStoreFor(
      primitives::BlockHeader const &block_header) const {
    if (block_header.number == 0) {
      return true;
    }

    BOOST_ASSERT_MSG(
        block_tree_.get()->getLastFinalized().number >= block_header.number,
        "Target block must be finalized");

    if (consensus::grandpa::HasAuthoritySetChange{block_header}) {
      return true;
    }
    if (block_header.number % 512 == 0) {
      return true;
    }
    return false;
  }

}  // namespace kagome::blockchain
