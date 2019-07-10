/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "basic_authorship/impl/block_builder_impl.hpp"

namespace kagome::basic_authorship {

  BlockBuilderImpl::BlockBuilderImpl(
      primitives::BlockHeader block_header,
      std::shared_ptr<runtime::BlockBuilderApi> r_block_builder)
      : block_header_(std::move(block_header)),
        r_block_builder_(std::move(r_block_builder)) {}

  outcome::result<bool> BlockBuilderImpl::pushExtrinsic(
      primitives::Extrinsic extrinsic) {
    OUTCOME_TRY(ok, r_block_builder_->apply_extrinsic(extrinsic));

    if (ok) {
      extrinsics_.push_back(extrinsic);
      return true;
    }
    return false;
  }

  primitives::Block BlockBuilderImpl::bake() const {
    return {block_header_, extrinsics_};
  }

}  // namespace kagome::basic_authorship
