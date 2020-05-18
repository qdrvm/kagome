/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/core_impl.hpp"

#include <boost/assert.hpp>

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::AuthorityId;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::Version;

  CoreImpl::CoreImpl(
      const std::shared_ptr<RuntimeManager> &runtime_manager,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker)
      : RuntimeApi(runtime_manager),
        changes_tracker_{std::move(changes_tracker)} {
    BOOST_ASSERT(changes_tracker_ != nullptr);
  }

  outcome::result<Version> CoreImpl::version() {
    return execute<Version>("Core_version", CallPersistency::EPHEMERAL);
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block) {
    return execute<void>(
        "Core_execute_block", CallPersistency::PERSISTENT, block);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    auto &&res = execute<void>(
        "Core_initialize_block", CallPersistency::PERSISTENT, header);
    if (res.has_value()) {
      OUTCOME_TRY(changes_tracker_->onBlockChange(header.parent_hash));
    }
    return res;
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<std::vector<AuthorityId>>(
        "Core_authorities", CallPersistency::EPHEMERAL, block_id);
  }
}  // namespace kagome::runtime::binaryen
