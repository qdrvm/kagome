/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_LIBP2P_CRYPTO_KEY_VALIDATOR_MOCK_HPP
#define KAGOME_TEST_MOCK_LIBP2P_CRYPTO_KEY_VALIDATOR_MOCK_HPP

#include "libp2p/crypto/key_validator.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace libp2p::crypto::validator {

  class KeyValidatorMock : public KeyValidator {
   public:
    ~KeyValidatorMock() override = default;

    MOCK_CONST_METHOD1(validate, outcome::result<void>(const PrivateKey &));

    MOCK_CONST_METHOD1(validate, outcome::result<void>(const PublicKey &));

    MOCK_CONST_METHOD1(validate, outcome::result<void>(const KeyPair &));
  };
}  // namespace libp2p::crypto::validator

#endif  // KAGOME_TEST_MOCK_LIBP2P_CRYPTO_KEY_VALIDATOR_MOCK_HPP
