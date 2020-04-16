/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "authorship/impl/block_builder_impl.hpp"

#include "authorship/impl/block_builder_error.hpp"

namespace kagome::authorship {

  BlockBuilderImpl::BlockBuilderImpl(
      primitives::BlockHeader block_header,
      std::shared_ptr<runtime::BlockBuilder> r_block_builder)
      : block_header_(std::move(block_header)),
        r_block_builder_(std::move(r_block_builder)),
        logger_{common::createLogger("BlockBuilder")} {
    BOOST_ASSERT(r_block_builder_ != nullptr);
  }

  outcome::result<void> BlockBuilderImpl::pushExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    auto apply_res = r_block_builder_->apply_extrinsic(extrinsic);
    if (not apply_res) {
      logger_->warn(
          "Extrinsic {} was not pushed to block. Error during xt application: "
          "{}",
          extrinsic.data.toHex(),
          apply_res.error().message());
      return apply_res.error();
    }

    const static std::string logger_error_template =
        "Extrinsic {} was not pushed to block. Extrinsic cannot be "
        "applied";
    return visit_in_place(
        apply_res.value(),
        [this, &extrinsic](
            primitives::ApplyOutcome apply_outcome) -> outcome::result<void> {
          switch (apply_outcome) {
            case primitives::ApplyOutcome::SUCCESS:
              extrinsics_.push_back(extrinsic);
              return outcome::success();
            case primitives::ApplyOutcome::FAIL:
              logger_->warn(logger_error_template, extrinsic.data.toHex());
              return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
          }
        },
        [this, &extrinsic](primitives::ApplyError) -> outcome::result<void> {
          logger_->warn(logger_error_template, extrinsic.data.toHex());
          return BlockBuilderError::EXTRINSIC_APPLICATION_FAILED;
        });
  }

  outcome::result<primitives::Block> BlockBuilderImpl::bake() const {
    OUTCOME_TRY(finalised_header, r_block_builder_->finalise_block());
    return primitives::Block{finalised_header, extrinsics_};
  }

}  // namespace kagome::authorship
