/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {
  struct SR25519ProviderMock : public SR25519Provider {
    MOCK_CONST_METHOD0(generateKeypair, SR25519Keypair());
    MOCK_CONST_METHOD1(generateKeypair, SR25519Keypair(common::Blob<32>));

    MOCK_CONST_METHOD2(sign,
                       outcome::result<SR25519Signature>(const SR25519Keypair &,
                                                         gsl::span<uint8_t>));

    MOCK_CONST_METHOD3(verify,
                       outcome::result<bool>(const SR25519Signature &,
                                             gsl::span<uint8_t>,
                                             const SR25519PublicKey &));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP
