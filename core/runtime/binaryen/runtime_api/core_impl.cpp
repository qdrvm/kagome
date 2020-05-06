/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/core_impl.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::AuthorityId;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::Version;

  CoreImpl::CoreImpl(const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(runtime_manager) {}

  outcome::result<Version> CoreImpl::version() {
    return execute<Version>("Core_version", CallPersistency::EPHEMERAL);
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block) {
    return execute<void>(
        "Core_execute_block", CallPersistency::PERSISTENT, block);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    return execute<void>(
        "Core_initialize_block", CallPersistency::PERSISTENT, header);
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<std::vector<AuthorityId>>(
        "Core_authorities", CallPersistency::EPHEMERAL, block_id);
  }
}  // namespace kagome::runtime::binaryen
