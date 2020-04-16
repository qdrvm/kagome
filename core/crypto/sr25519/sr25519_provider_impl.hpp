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

#ifndef KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP

#include "crypto/random_generator.hpp"
#include "crypto/sr25519_provider.hpp"

namespace libp2p::crypto::random {
  class CSPRNG;
}

namespace kagome::crypto {

  class SR25519ProviderImpl : public SR25519Provider {
    using CSPRNG = libp2p::crypto::random::CSPRNG;

   public:
    explicit SR25519ProviderImpl(std::shared_ptr<CSPRNG> generator);

    ~SR25519ProviderImpl() override = default;

    SR25519Keypair generateKeypair() const override;

    outcome::result<SR25519Signature> sign(
        const SR25519Keypair &keypair,
        gsl::span<uint8_t> message) const override;

    outcome::result<bool> verify(
        const SR25519Signature &signature,
        gsl::span<uint8_t> message,
        const SR25519PublicKey &public_key) const override;

   private:
    std::shared_ptr<CSPRNG> generator_;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP
