/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_BLOCK_BUILDER_HPP
#define KAGOME_RUNTIME_IMPL_BLOCK_BUILDER_HPP

#include "runtime/runtime_api/block_builder.hpp"

namespace kagome::runtime {

  class Executor;

  class BlockBuilderImpl final : public BlockBuilder {
   public:
    explicit BlockBuilderImpl(std::shared_ptr<Executor> executor);

    outcome::result<primitives::ApplyExtrinsicResult> apply_extrinsic(
        RuntimeEnvironment &env,
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::BlockHeader> finalize_block(
        RuntimeEnvironment &env) override;

    outcome::result<std::vector<primitives::Extrinsic>> inherent_extrinsics(
        RuntimeEnvironment &env, const primitives::InherentData &data) override;

    outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) override;

    outcome::result<common::Hash256> random_seed(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_BLOCK_BUILDER_HPP
