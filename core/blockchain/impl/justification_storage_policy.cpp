/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/justification_storage_policy.hpp"

#include <memory>

#include "consensus/authority/authority_manager.hpp"

namespace kagome::blockchain {

  bool JustificationStoragePolicyImpl::shouldStore(
      primitives::BlockInfo block) const {
    BOOST_ASSERT_MSG(auth_manager_ != nullptr,
                     "Authority manager should be initialized with "
                     "JustificationStoragePolicyImpl::initAuthorityManager()");
    bool authority_set_change = false;
    auto prev_block_auth = auth_manager_->authorities(block, false);
    auto this_block_auth = auth_manager_->authorities(block, false);
    // if authority set was updated in previous block
    if (prev_block_auth.has_value() && this_block_auth.has_value()
        && prev_block_auth.value()->id != this_block_auth.value()->id) {
      authority_set_change = true;
    } else {
      auto next_block_auth = auth_manager_->authorities(block, false);
      // if authority set was updated in this block
      if (next_block_auth.has_value() && this_block_auth.has_value()
          && next_block_auth.value()->id != this_block_auth.value()->id) {
        authority_set_change = true;
      }
    }
    return block.number % 512 == 0 or authority_set_change;
  }

  void JustificationStoragePolicyImpl::initAuthorityManager(
      std::shared_ptr<const authority::AuthorityManager> auth_manager) {
    BOOST_ASSERT_MSG(auth_manager_ == nullptr,
                     "Authority manager should be initialized once");
    BOOST_ASSERT(auth_manager != nullptr);
    auth_manager_ = auth_manager;
  }

}  // namespace kagome::blockchain