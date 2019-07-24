/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_impl.hpp"

#include "authorship/impl/block_builder_error.hpp"

namespace kagome::authorship {

  BlockBuilderImpl::BlockBuilderImpl(
      primitives::BlockHeader block_header,
      std::shared_ptr<runtime::BlockBuilderApi> r_block_builder,
      common::Logger logger)
      : block_header_(std::move(block_header)),
        r_block_builder_(std::move(r_block_builder)),
        logger_(std::move(logger)) {
    BOOST_ASSERT(r_block_builder_ != nullptr);
    BOOST_ASSERT(logger_);
  }

  outcome::result<void> BlockBuilderImpl::pushExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    OUTCOME_TRY(ok, r_block_builder_->apply_extrinsic(extrinsic));

    if (ok) {
      extrinsics_.push_back(extrinsic);
      return outcome::success();
    }
    logger_->warn(
        "Extrinsic {} was not pushed to block. Extrinsic cannot be applied",
        extrinsic.data.toHex());
    return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
  }

  outcome::result<primitives::Block> BlockBuilderImpl::bake() const {
    OUTCOME_TRY(finalised_header, r_block_builder_->finalise_block());
    return primitives::Block{finalised_header, extrinsics_};
  }

}  // namespace kagome::authorship
