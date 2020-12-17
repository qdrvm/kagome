/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_JUSTIFICATIONVALIDATOR
#define KAGOME_CONSENSUS_JUSTIFICATIONVALIDATOR

#include <outcome/outcome.hpp>
#include "primitives/common.hpp"
#include "primitives/justification.hpp"

namespace kagome::consensus {
  /**
   * Validator of justification of the blocks
   */
  class JustificationValidator {
   public:
    virtual ~JustificationValidator() = default;

    virtual outcome::result<void> validateJustification(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) const = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_JUSTIFICATIONVALIDATOR
