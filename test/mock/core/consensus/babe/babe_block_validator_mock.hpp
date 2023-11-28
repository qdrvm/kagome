/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_block_validator.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BabeBlockValidatorMock : public BabeBlockValidator {
   public:
    MOCK_METHOD(outcome::result<void>,
                validateHeader,
                (const primitives::BlockHeader &block_header),
                (const, override));
  };

}  // namespace kagome::consensus::babe
