/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_JUSTIFICATIONVALIDATORMOCK
#define KAGOME_CONSENSUS_JUSTIFICATIONVALIDATORMOCK

#include "consensus/validation/justification_validator.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class JustificationValidatorMock : public JustificationValidator {
   public:
    MOCK_CONST_METHOD2(
        validateJustification,
        outcome::result<void>(const primitives::BlockHash &,
                              const primitives::Justification &));
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_JUSTIFICATIONVALIDATORMOCK
