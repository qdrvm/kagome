/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/validator/signer.hpp"

namespace kagome::parachain {

  class ValidatorSignerFactoryMock : public IValidatorSignerFactory {
   public:
    MOCK_METHOD(outcome::result<std::optional<ValidatorSigner>>,
                at,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<ValidatorIndex>>,
                getAuthorityValidatorIndex,
                (const primitives::BlockHash &),
                (override));
  };

}  // namespace kagome::parachain
