/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/core_impl.hpp"

#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using extensions::Extension;
  using primitives::AuthorityId;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::Version;

  CoreImpl::CoreImpl(Buffer state_code, std::shared_ptr<Extension> extension) {
    runtime_ = std::make_unique<RuntimeApi>(std::move(state_code),
                                            std::move(extension));
  }

  CoreImpl::~CoreImpl() {}

  outcome::result<Version> CoreImpl::version() {
    return runtime_->execute<Version>("Core_version");
  }

  outcome::result<void> CoreImpl::execute_block(const Block &block) {
    return runtime_->execute("Core_execute_block", block);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    return runtime_->execute("Core_initialise_block", header);
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities() {
    return runtime_->execute<std::vector<AuthorityId>>("Core_authorities");
  }
}  // namespace kagome::runtime
