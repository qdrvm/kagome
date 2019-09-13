/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "crypto/sr25519_types.hpp"
#include "libp2p/crypto/random_generator.hpp"

namespace kagome::crypto {
  SR25519ProviderImpl::SR25519ProviderImpl(std::shared_ptr<CSPRNG> generator)
      : generator_(std::move(generator)) {}

  SR25519Keypair SR25519ProviderImpl::generateKeypair() const {
    auto seed = generator_->randomBytes(constants::sr25519::SEED_SIZE);

    std::vector<uint8_t> kp(constants::sr25519::KEYPAIR_SIZE, 0);
    sr25519_keypair_from_seed(kp.data(), seed.data());

    return SR25519Keypair{kp};
  }

  SR25519Signature SR25519ProviderImpl::sign(const SR25519Keypair &keypair,
                                             gsl::span<uint8_t> message) const {
    SR25519Signature signature{};

    sr25519_sign(signature.data(),
                 keypair.public_key.data(),
                 keypair.secret_key.data(),
                 message.data(),
                 message.size());

    return signature;
  }

  bool SR25519ProviderImpl::verify(const SR25519Signature &signature,
                                   gsl::span<uint8_t> message,
                                   const SR25519PublicKey &public_key) const {
    return sr25519_verify(
        signature.data(), message.data(), message.size(), public_key.data());
  }
}  // namespace kagome::crypto
