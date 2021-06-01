/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_BLOCK_BUILDER_HPP
#define KAGOME_RUNTIME_WAVM_BLOCK_BUILDER_HPP

#include "runtime/block_builder.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmBlockBuilder final: public BlockBuilder {
   public:
    WavmBlockBuilder(std::shared_ptr<Executor> executor);

    outcome::result<primitives::ApplyResult> apply_extrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::BlockHeader> finalise_block() override;

    outcome::result<std::vector<primitives::Extrinsic>>
    inherent_extrinsics(const primitives::InherentData &data) override;

    outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) override;

    outcome::result<common::Hash256> random_seed() override;

   private:
    std::shared_ptr<Executor> executor_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_BLOCK_BUILDER_HPP
