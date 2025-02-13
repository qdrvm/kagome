/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/validator/signer.hpp"

namespace kagome::parachain {

  class ValidatorSignerMock : public IValidatorSigner {
   public:
    MOCK_METHOD(outcome::result<IndexedAndSigned<network::Statement>>,
                sign,
                (const network::Statement &),
                (const, override));

    MOCK_METHOD(outcome::result<IndexedAndSigned<scale::BitVec>>,
                sign,
                (const scale::BitVec &),
                (const, override));

    MOCK_METHOD(ValidatorIndex, validatorIndex, (), (const, override));

    MOCK_METHOD(SessionIndex, getSessionIndex, (), (const, override));

    MOCK_METHOD(const primitives::BlockHash &,
                relayParent,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<Signature>,
                signRaw,
                (common::BufferView),
                (const, override));
  };

  class ValidatorSignerFactoryMock : public IValidatorSignerFactory {
   public:
    MOCK_METHOD(
        outcome::result<std::optional<std::shared_ptr<IValidatorSigner>>>,
        at,
        (const primitives::BlockHash &),
        (override));

    MOCK_METHOD(outcome::result<std::optional<ValidatorIndex>>,
                getAuthorityValidatorIndex,
                (const primitives::BlockHash &),
                (override));
  };

}  // namespace kagome::parachain
