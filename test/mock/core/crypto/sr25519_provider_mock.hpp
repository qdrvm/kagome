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

#ifndef KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {
  struct SR25519ProviderMock : public SR25519Provider {
    MOCK_CONST_METHOD0(generateKeypair, SR25519Keypair());

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
