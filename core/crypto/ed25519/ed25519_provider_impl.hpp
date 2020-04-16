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

#ifndef KAGOME_CORE_CRYPTO_ED25519_ED25519_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CRYPTO_ED25519_ED25519_PROVIDER_IMPL_HPP

#include "crypto/ed25519_provider.hpp"

namespace kagome::crypto {

  class ED25519ProviderImpl : public ED25519Provider {
   public:
    ~ED25519ProviderImpl() override = default;

    outcome::result<ED25519Keypair> generateKeypair() const override;

    outcome::result<kagome::crypto::ED25519Signature> sign(
        const ED25519Keypair &keypair,
        gsl::span<uint8_t> message) const override;

    outcome::result<bool> verify(
        const ED25519Signature &signature,
        gsl::span<uint8_t> message,
        const ED25519PublicKey &public_key) const override;
  };

}  // namespace kagome::crypto

#endif  //KAGOME_CORE_CRYPTO_ED25519_ED25519_PROVIDER_IMPL_HPP

