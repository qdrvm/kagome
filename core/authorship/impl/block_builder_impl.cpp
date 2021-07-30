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

    using return_type = outcome::result<primitives::ExtrinsicIndex>;
    return visit_in_place(
        apply_res.value(),
        [this, &extrinsic](
            const primitives::DispatchOutcome &outcome) -> return_type {
          if (1 == outcome.which()) {  // DispatchError
            SL_WARN(
                logger_,
                "Extrinsic {} pushed to the block, but dispatch error occurred",
                extrinsic.data.toHex().substr(0, 8));
          }
          extrinsics_.push_back(extrinsic);
          return extrinsics_.size() - 1;
        },
        [this, &extrinsic](const primitives::TransactionValidityError &tx_error)
            -> return_type {
          return visit_in_place(
              tx_error,
              [this, &extrinsic](
                  const primitives::InvalidTransaction &reason) -> return_type {
                switch (reason) {
                  case primitives::InvalidTransaction::ExhaustsResources:
                    return BlockBuilderError::EXHAUSTS_RESOURCES;
                  case primitives::InvalidTransaction::BadMandatory:
                    return BlockBuilderError::BAD_MANDATORY;
                  default:
                    SL_WARN(
                        logger_,
                        "Extrinsic {} cannot be applied and was not pushed to "
                        "the block. (InvalidTransaction response, code {})",
                        extrinsic.data.toHex().substr(0, 8),
                        reason);
                    return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
                }
              },
              [this, &extrinsic](
                  const primitives::UnknownTransaction &reason) -> return_type {
                SL_WARN(logger_,
                        "Extrinsic {} cannot be applied and was not pushed to "
                        "the block. (UnknownTransaction response, code {})",
                        extrinsic.data.toHex().substr(0, 8),
                        reason);
                return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
              });
        });
  }

  outcome::result<primitives::Block> BlockBuilderImpl::bake() const {
    OUTCOME_TRY(finalised_header, block_builder_api_->finalise_block());
    return primitives::Block{finalised_header, extrinsics_};
  }

}  // namespace kagome::authorship
