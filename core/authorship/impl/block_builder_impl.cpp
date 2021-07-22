/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_impl.hpp"

#include "authorship/impl/block_builder_error.hpp"
#include "common/visitor.hpp"

namespace kagome::authorship {

  using primitives::events::ExtrinsicEventType;
  using primitives::events::ExtrinsicSubscriptionEngine;
  using primitives::events::InBlockEventParams;

  BlockBuilderImpl::BlockBuilderImpl(
      primitives::BlockHeader block_header,
      std::shared_ptr<runtime::BlockBuilder> block_builder_api)
      : block_header_(std::move(block_header)),
        block_builder_api_(std::move(block_builder_api)),
        logger_{log::createLogger("BlockBuilder", "authorship")} {
    BOOST_ASSERT(block_builder_api_ != nullptr);
  }

  outcome::result<primitives::ExtrinsicIndex> BlockBuilderImpl::pushExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    auto apply_res = block_builder_api_->apply_extrinsic(extrinsic);
    if (not apply_res) {
      // Takes place when API method execution fails for some technical kind of
      // problem. This WON'T be executed when apply_extrinsic returned an error
      // regarding the business-logic part.
      logger_->warn(
          "Extrinsic {} was not pushed to block. Error during xt application: "
          "{}",
          extrinsic.data.toHex().substr(0, 8),
          apply_res.error().message());
      return apply_res.error();
    }

    const static std::string logger_error_template =
        "Extrinsic {} was not pushed to block. Extrinsic cannot be "
        "applied";
    return visit_in_place(
        apply_res.value(),
        [this, &extrinsic](primitives::ApplyOutcome apply_outcome)
            -> outcome::result<primitives::ExtrinsicIndex> {
          switch (apply_outcome) {
            case primitives::ApplyOutcome::FAIL:
              logger_->warn(logger_error_template,
                            extrinsic.data.toHex().substr(0, 8));
              [[fallthrough]];
            case primitives::ApplyOutcome::SUCCESS:
              extrinsics_.push_back(extrinsic);
              return extrinsics_.size() - 1;
          }
          // Not going to happen
          throw std::runtime_error("Not all ApplyOutcome cases are checked");
        },
        [this, &extrinsic](primitives::ApplyError)
            -> outcome::result<primitives::ExtrinsicIndex> {
          logger_->warn(logger_error_template,
                        extrinsic.data.toHex().substr(0, 8));
          return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
        });
  }

  outcome::result<primitives::Block> BlockBuilderImpl::bake() const {
    OUTCOME_TRY(finalised_header, block_builder_api_->finalise_block());
    return primitives::Block{finalised_header, extrinsics_};
  }

}  // namespace kagome::authorship
