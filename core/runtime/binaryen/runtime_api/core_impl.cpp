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

  CoreImpl::CoreImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

  outcome::result<Version> CoreImpl::version() {
    return execute<Version>("Core_version");
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::BlockData &block_data) {
    return execute<void>("Core_execute_block", block_data);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    return execute<void>("Core_initialize_block", header);
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<std::vector<AuthorityId>>("Core_authorities", block_id);
  }
}  // namespace kagome::runtime::binaryen
