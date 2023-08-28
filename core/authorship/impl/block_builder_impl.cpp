/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_impl.hpp"

#include "authorship/impl/block_builder_error.hpp"
#include "common/visitor.hpp"
#include "primitives/transaction_validity.hpp"

namespace kagome::authorship {

  using primitives::events::ExtrinsicEventType;
  using primitives::events::ExtrinsicSubscriptionEngine;
  using primitives::events::InBlockEventParams;

  BlockBuilderImpl::BlockBuilderImpl(
      primitives::BlockHeader block_header,
      std::unique_ptr<runtime::RuntimeContext> ctx,
      std::shared_ptr<runtime::BlockBuilder> block_builder_api)
      : block_header_{std::move(block_header)},
        block_builder_api_{std::move(block_builder_api)},
        ctx_{std::move(ctx)},
        logger_{log::createLogger("BlockBuilder", "authorship")} {
    BOOST_ASSERT(block_builder_api_ != nullptr);
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  BlockBuilderImpl::getInherentExtrinsics(
      const primitives::InherentData &data) const {
    return block_builder_api_->inherent_extrinsics(*ctx_, data);
  }

  outcome::result<primitives::ExtrinsicIndex> BlockBuilderImpl::pushExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    auto apply_res = block_builder_api_->apply_extrinsic(*ctx_, extrinsic);
    if (not apply_res) {
      // Takes place when API method execution fails for some technical kind of
      // problem. This WON'T be executed when apply_extrinsic returned an error
      // regarding the business-logic part.
      logger_->warn(
          "Extrinsic {} was not pushed to block. Error during xt application: "
          "{}",
          extrinsic.data.toHex().substr(0, 8),
          apply_res.error());
      return apply_res.error();
    }

    using return_type = outcome::result<primitives::ExtrinsicIndex>;
    return visit_in_place(
        apply_res.value(),
        [this, &extrinsic](
            const primitives::DispatchOutcome &outcome) -> return_type {
          if (auto error = boost::get<primitives::DispatchError>(&outcome)) {
            SL_WARN(logger_,
                    "Extrinsic {} cannot be applied and was not pushed to the "
                    "block. (DispatchError {})",
                    extrinsic.data.toHex().substr(0, 8),
                    error->which());
            return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
          }
          // https://github.com/paritytech/substrate/blob/943c520aa78fcfaf3509790009ad062e8d4c6990/client/block-builder/src/lib.rs#L204-L237
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
                        static_cast<uint8_t>(reason));
                    return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
                }
              },
              [this, &extrinsic](
                  const primitives::UnknownTransaction &reason) -> return_type {
                SL_WARN(logger_,
                        "Extrinsic {} cannot be applied and was not pushed to "
                        "the block. (UnknownTransaction response, code {})",
                        extrinsic.data.toHex().substr(0, 8),
                        static_cast<uint8_t>(reason));
                return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
              });
        });
  }

  outcome::result<primitives::Block> BlockBuilderImpl::bake() const {
    OUTCOME_TRY(finalized_header, block_builder_api_->finalize_block(*ctx_));
    return primitives::Block{finalized_header, extrinsics_};
  }

  size_t BlockBuilderImpl::estimateBlockSize() const {
    scale::ScaleEncoderStream s(true);
    for (const auto &xt : extrinsics_) {
      s << xt;
    }
    return estimatedBlockHeaderSize() + s.size();
  }

  size_t BlockBuilderImpl::estimatedBlockHeaderSize() const {
    static std::optional<size_t> size = std::nullopt;
    if (not size) {
      scale::ScaleEncoderStream s(true);
      s << block_header_;
      size = s.size();
    }
    return size.value();
  }
}  // namespace kagome::authorship
