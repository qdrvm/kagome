/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/justification_storage_policy.hpp"

#include <memory>

#include "blockchain/block_tree.hpp"
#include "consensus/authority/authority_manager.hpp"

namespace kagome::blockchain {

  outcome::result<std::vector<primitives::BlockNumber>>
  JustificationStoragePolicyImpl::shouldStoreWhatWhenFinalized(
      primitives::BlockInfo block) const {
    if (block.number == 0) return std::vector<primitives::BlockNumber>{0};

    BOOST_ASSERT_MSG(block_tree_ != nullptr,
                     "Block tree must have been initialized with "
                     "JustificationStoragePolicyImpl::initBlockchainInfo()");

    auto last_finalized = block_tree_->getLastFinalized();
    BOOST_ASSERT_MSG(last_finalized.number >= block.number,
                     "Target block must be finalized");
    BOOST_ASSERT_MSG(auth_manager_ != nullptr,
                     "Authority manager must have been initialized with "
                     "JustificationStoragePolicyImpl::initBlockchainInfo()");

    OUTCOME_TRY(block_header, block_tree_->getBlockHeader(block.hash));
    primitives::BlockInfo parent_block{block.number - 1,
                                       block_header.parent_hash};
    auto prev_block_auth = auth_manager_->authorities(last_finalized, true);
    auto this_block_auth = auth_manager_->authorities(block, true);
    // if authority set was updated between finalizations
    if (prev_block_auth.has_value() && this_block_auth.has_value()
        && prev_block_auth.value()->id != this_block_auth.value()->id) {
      return std::vector<primitives::BlockNumber>{last_finalized.number,
                                                  block.number};
    }
    if (block.number % 512 == 0) {
      return std::vector<primitives::BlockNumber>{block.number};
    }
    return {{}};
  }

  void JustificationStoragePolicyImpl::initBlockchainInfo(
      std::shared_ptr<const authority::AuthorityManager> auth_manager,
      std::shared_ptr<const blockchain::BlockTree> block_tree) {
    BOOST_ASSERT_MSG(auth_manager_ == nullptr,
                     "Authority manager should be initialized once");
    BOOST_ASSERT_MSG(block_tree_ == nullptr,
                     "Block tree should be initialized once");
    BOOST_ASSERT(auth_manager != nullptr);
    BOOST_ASSERT(block_tree != nullptr);
    auth_manager_ = auth_manager;
    block_tree_ = block_tree;
  }

}  // namespace kagome::blockchain