/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_JUSTIFICATIONVALIDATORIMPL
#define KAGOME_CONSENSUS_JUSTIFICATIONVALIDATORIMPL

#include "consensus/validation/justification_validator.hpp"

#include "consensus/grandpa/environment.hpp"

namespace kagome::consensus {
  /**
   * Validator of justification of the blocks
   */
  class JustificationValidatorImpl : public JustificationValidator {
   public:
    JustificationValidatorImpl(
        std::shared_ptr<grandpa::Environment> grandpa_environment);
    JustificationValidatorImpl() = default;

    outcome::result<void> validateJustification(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) const override;

   private:
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_JUSTIFICATIONVALIDATORIMPL
