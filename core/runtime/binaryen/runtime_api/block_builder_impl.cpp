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
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory)
      : RuntimeApi(runtime_env_factory) {}

  outcome::result<primitives::ApplyExtrinsicResult>
  BlockBuilderImpl::apply_extrinsic(const Extrinsic &extrinsic) {
    return execute<primitives::ApplyExtrinsicResult>(
        "BlockBuilder_apply_extrinsic",
        CallConfig{.persistency = CallPersistency::PERSISTENT},
        extrinsic);
  }

  outcome::result<BlockHeader> BlockBuilderImpl::finalise_block() {
    return execute<BlockHeader>(
        "BlockBuilder_finalize_block",
        CallConfig{.persistency = CallPersistency::PERSISTENT});
  }

  outcome::result<std::vector<Extrinsic>> BlockBuilderImpl::inherent_extrinsics(
      const InherentData &data) {
    return execute<std::vector<Extrinsic>>(
        "BlockBuilder_inherent_extrinsics",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        data);
  }

  outcome::result<CheckInherentsResult> BlockBuilderImpl::check_inherents(
      const Block &block, const InherentData &data) {
    return execute<CheckInherentsResult>(
        "BlockBuilder_check_inherents",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        block,
        data);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed() {
    // TODO(Harrm) PRE-154 Figure out what it requires
    return execute<common::Hash256>(
        "BlockBuilder_random_seed",
        CallConfig{.persistency = CallPersistency::EPHEMERAL});
  }
}  // namespace kagome::runtime::binaryen
