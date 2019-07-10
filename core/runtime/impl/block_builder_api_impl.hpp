/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_BUILDER_API_IMPL_HPP
#define KAGOME_BLOCK_BUILDER_API_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/block_builder_api.hpp"
#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  class BlockBuilderImpl : public RuntimeApi, public BlockBuilderApi {
   public:
    BlockBuilderImpl(common::Buffer state_code,
                     std::shared_ptr<extensions::Extension> extension);

    ~BlockBuilderImpl() override = default;

    outcome::result<bool> apply_extrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::BlockHeader> finalize_block() override;

    outcome::result<std::vector<primitives::Extrinsic>> inherent_extrinsics(
        const primitives::InherentData &data) override;

    outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) override;

    outcome::result<common::Hash256> random_seed() override;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_BLOCK_BUILDER_API_IMPL_HPP
