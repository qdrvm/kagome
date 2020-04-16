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

#include "runtime/binaryen/runtime_api/block_builder_impl.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using extensions::Extension;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::CheckInherentsResult;
  using primitives::Extrinsic;
  using primitives::InherentData;

  BlockBuilderImpl::BlockBuilderImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

  outcome::result<primitives::ApplyResult> BlockBuilderImpl::apply_extrinsic(
      const Extrinsic &extrinsic) {
    return execute<primitives::ApplyResult>("BlockBuilder_apply_extrinsic",
                                            extrinsic);
  }

  outcome::result<BlockHeader> BlockBuilderImpl::finalise_block() {
    // TODO(Harrm) PRE-154 figure out what wasm function returns
    return execute<BlockHeader>("BlockBuilder_finalize_block");
  }

  outcome::result<std::vector<Extrinsic>>
  BlockBuilderImpl::inherent_extrinsics(const InherentData &data) {
    return execute<std::vector<Extrinsic>>("BlockBuilder_inherent_extrinsics",
                                           data);
  }

  outcome::result<CheckInherentsResult> BlockBuilderImpl::check_inherents(
      const Block &block, const InherentData &data) {
    return execute<CheckInherentsResult>(
        "BlockBuilder_check_inherents", block, data);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed() {
    // TODO(Harrm) PRE-154 Figure out what it requires
    return execute<common::Hash256>("BlockBuilder_random_seed");
  }
}  // namespace kagome::runtime
