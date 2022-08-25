/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/block_builder.hpp"

#include "runtime/common/executor.hpp"

namespace kagome::runtime {

  BlockBuilderImpl::BlockBuilderImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<PersistentResult<primitives::ApplyExtrinsicResult>>
  BlockBuilderImpl::apply_extrinsic(const primitives::BlockInfo &block,
                                    storage::trie::RootHash const &storage_hash,
                                    const primitives::Extrinsic &extrinsic) {
    return executor_->persistentCallAt<primitives::ApplyExtrinsicResult>(
        block, storage_hash, "BlockBuilder_apply_extrinsic", extrinsic);
  }

  outcome::result<primitives::BlockHeader> BlockBuilderImpl::finalize_block(
      const primitives::BlockInfo &block,
      storage::trie::RootHash const &storage_hash) {
    const auto res = executor_->persistentCallAt<primitives::BlockHeader>(
        block, storage_hash, "BlockBuilder_finalize_block");
    if (res) return res.value().result;
    return res.error();
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  BlockBuilderImpl::inherent_extrinsics(
      const primitives::BlockInfo &block,
      storage::trie::RootHash const &storage_hash,
      const primitives::InherentData &data) {
    return executor_->callAt<std::vector<primitives::Extrinsic>>(
        block, storage_hash, "BlockBuilder_inherent_extrinsics", data);
  }

  outcome::result<primitives::CheckInherentsResult>
  BlockBuilderImpl::check_inherents(const primitives::Block &block,
                                    const primitives::InherentData &data) {
    return executor_->callAt<primitives::CheckInherentsResult>(
        block.header.parent_hash, "BlockBuilder_check_inherents", block, data);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed(
      const primitives::BlockHash &block) {
    return executor_->callAt<common::Hash256>(block,
                                              "BlockBuilder_random_seed");
  }

}  // namespace kagome::runtime
