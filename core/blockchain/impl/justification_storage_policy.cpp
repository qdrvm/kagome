/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/justification_storage_policy.hpp"

#include <memory>

#include "blockchain/block_tree.hpp"
#include "primitives/digest.hpp"

namespace kagome::blockchain {

  outcome::result<bool> JustificationStoragePolicyImpl::shouldStoreFor(
      primitives::BlockHeader const &block_header) const {
    if (block_header.number == 0) {
      return true;
    }

    BOOST_ASSERT_MSG(block_tree_ != nullptr,
                     "Block tree must have been initialized with "
                     "JustificationStoragePolicyImpl::initBlockchainInfo()");

    BOOST_ASSERT_MSG(
        block_tree_->getLastFinalized().number >= block_header.number,
        "Target block must be finalized");

    for (auto &digest : block_header.digest) {
      if (auto *consensus_digest_enc =
              boost::get<primitives::Consensus>(&digest);
          consensus_digest_enc != nullptr) {
        OUTCOME_TRY(consensus_digest, consensus_digest_enc->decode());
        bool authority_change =
            consensus_digest.isGrandpaDigestOf<primitives::ScheduledChange>()
            || consensus_digest.isGrandpaDigestOf<primitives::ForcedChange>();
        if (authority_change) {
          return true;
        }
      }
    }
    if (block_header.number % 512 == 0) {
      return true;
    }
    return false;
  }

  void JustificationStoragePolicyImpl::initBlockchainInfo(
      std::shared_ptr<const blockchain::BlockTree> block_tree) {
    BOOST_ASSERT_MSG(block_tree_ == nullptr,
                     "Block tree should be initialized once");
    BOOST_ASSERT(block_tree != nullptr);
    block_tree_ = block_tree;
  }

}  // namespace kagome::blockchain
