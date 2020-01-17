/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/block_builder_api_impl.hpp"

#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using extensions::Extension;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::CheckInherentsResult;
  using primitives::Extrinsic;
  using primitives::InherentData;

  BlockBuilderApiImpl::BlockBuilderApiImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

  outcome::result<primitives::ApplyResult> BlockBuilderApiImpl::apply_extrinsic(
      const Extrinsic &extrinsic) {
    return execute<primitives::ApplyResult>("BlockBuilder_apply_extrinsic",
                                            extrinsic);
  }

  outcome::result<BlockHeader> BlockBuilderApiImpl::finalise_block() {
    // TODO(Harrm) PRE-154 figure out what wasm function returns
    return execute<BlockHeader>("BlockBuilder_finalize_block");
  }

  outcome::result<std::vector<Extrinsic>>
  BlockBuilderApiImpl::inherent_extrinsics(const InherentData &data) {
    return execute<std::vector<Extrinsic>>("BlockBuilder_inherent_extrinsics",
                                           data);
  }

  outcome::result<CheckInherentsResult> BlockBuilderApiImpl::check_inherents(
      const Block &block, const InherentData &data) {
    return execute<CheckInherentsResult>(
        "BlockBuilder_check_inherents", block, data);
  }

  outcome::result<common::Hash256> BlockBuilderApiImpl::random_seed() {
    // TODO(Harrm) PRE-154 Figure out what it requires
    return execute<common::Hash256>("BlockBuilder_random_seed");
  }
}  // namespace kagome::runtime
