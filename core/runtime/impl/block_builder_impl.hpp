/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_BUILDER_IMPL_HPP
#define KAGOME_BLOCK_BUILDER_IMPL_HPP

#include "extensions/extension.hpp"
#include "primitives/scale_codec.hpp"
#include "runtime/block_builder.hpp"
#include "runtime/wasm_memory.hpp"
#include "wasm_executor.hpp"

namespace kagome::runtime {

  class BlockBuilderImpl : public BlockBuilder {
   public:
    BlockBuilderImpl(common::Buffer state_code,
                     std::shared_ptr<extensions::Extension> extension,
                     std::shared_ptr<primitives::ScaleCodec> codec);

    ~BlockBuilderImpl() override = default;

    outcome::result<bool> apply_extrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::BlockHeader> finalize_block() override;

    outcome::result<std::vector<primitives::Extrinsic>>
    inherent_extrinsics(
        const primitives::InherentData &data) override;

    outcome::result<CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) override;

    outcome::result<common::Hash256> random_seed() override;

   private:
    std::shared_ptr<WasmMemory> memory_;
    std::shared_ptr<primitives::ScaleCodec> codec_;
    WasmExecutor executor_;
    common::Buffer state_code_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_BLOCK_BUILDER_IMPL_HPP
