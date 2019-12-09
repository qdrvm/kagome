/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/core_impl.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using extensions::Extension;
  using primitives::AuthorityId;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::Version;

  CoreImpl::CoreImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<Extension> &extension)
      : RuntimeApi(wasm_provider, extension) {}

  outcome::result<Version> CoreImpl::version() {
    return execute<Version>("Core_version");
  }

  outcome::result<void> CoreImpl::execute_block(const Block &block) {
    return execute<void>("Core_execute_block", block);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    return execute<void>("Core_initialise_block", header);
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities() {
    return execute<std::vector<AuthorityId>>("Core_authorities");
  }
}  // namespace kagome::runtime
