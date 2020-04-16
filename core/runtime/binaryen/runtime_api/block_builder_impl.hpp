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

#ifndef KAGOME_RUNTIME_BINARYEN_BLOCK_BUILDER_IMPL_HPP
#define KAGOME_RUNTIME_BINARYEN_BLOCK_BUILDER_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/block_builder.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime::binaryen {

  class BlockBuilderImpl : public RuntimeApi, public BlockBuilder {
   public:
    BlockBuilderImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::ExtensionFactory> &extension_factory);

    ~BlockBuilderImpl() override = default;

    outcome::result<primitives::ApplyResult> apply_extrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::BlockHeader> finalise_block() override;

    outcome::result<std::vector<primitives::Extrinsic>> inherent_extrinsics(
        const primitives::InherentData &data) override;

    outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) override;

    outcome::result<common::Hash256> random_seed() override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_RUNTIME_BINARYEN_BLOCK_BUILDER_IMPL_HPP
