/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
      const primitives::Block &block) {
    return execute<void>("Core_execute_block", block);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    return execute<void>("Core_initialize_block", header);
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<std::vector<AuthorityId>>("Core_authorities", block_id);
  }
}  // namespace kagome::runtime::binaryen
