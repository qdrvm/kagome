/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_BINARYEN_BLOCK_BUILDER_IMPL_HPP
#define KAGOME_RUNTIME_BINARYEN_BLOCK_BUILDER_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/block_builder.hpp"

namespace kagome::runtime::binaryen {

  class BlockBuilderImpl : public RuntimeApi, public BlockBuilder {
   public:
    explicit BlockBuilderImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory);

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
