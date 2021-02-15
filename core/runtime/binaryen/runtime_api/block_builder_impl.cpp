/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/block_builder_impl.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using host_api::HostApi;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::CheckInherentsResult;
  using primitives::Extrinsic;
  using primitives::InherentData;

  BlockBuilderImpl::BlockBuilderImpl(
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_manager)
      : RuntimeApi(runtime_manager) {}

  outcome::result<primitives::ApplyResult> BlockBuilderImpl::apply_extrinsic(
      const Extrinsic &extrinsic) {
    return execute<primitives::ApplyResult>(
        "BlockBuilder_apply_extrinsic", CallPersistency::PERSISTENT, extrinsic);
  }

  outcome::result<BlockHeader> BlockBuilderImpl::finalise_block() {
    return execute<BlockHeader>("BlockBuilder_finalize_block",
                                CallPersistency::PERSISTENT);
  }

  outcome::result<std::vector<Extrinsic>> BlockBuilderImpl::inherent_extrinsics(
      const InherentData &data) {
    return execute<std::vector<Extrinsic>>(
        "BlockBuilder_inherent_extrinsics", CallPersistency::EPHEMERAL, data);
  }

  outcome::result<CheckInherentsResult> BlockBuilderImpl::check_inherents(
      const Block &block, const InherentData &data) {
    return execute<CheckInherentsResult>("BlockBuilder_check_inherents",
                                         CallPersistency::EPHEMERAL,
                                         block,
                                         data);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed() {
    // TODO(Harrm) PRE-154 Figure out what it requires
    return execute<common::Hash256>("BlockBuilder_random_seed",
                                    CallPersistency::EPHEMERAL);
  }
}  // namespace kagome::runtime::binaryen
