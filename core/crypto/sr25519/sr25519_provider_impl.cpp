/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "crypto/sr25519_types.hpp"
#include "libp2p/crypto/random_generator.hpp"

namespace kagome::crypto {
  SR25519ProviderImpl::SR25519ProviderImpl(std::shared_ptr<CSPRNG> generator)
      : generator_(std::move(generator)) {
    BOOST_ASSERT(generator_ != nullptr);
  }

  SR25519Keypair SR25519ProviderImpl::generateKeypair() const {
    auto seed = generator_->randomBytes(constants::sr25519::SEED_SIZE);

    std::vector<uint8_t> kp(constants::sr25519::KEYPAIR_SIZE, 0);
    sr25519_keypair_from_seed(kp.data(), seed.data());

    return SR25519Keypair{kp};
  }

  outcome::result<SR25519Signature> SR25519ProviderImpl::sign(
      const SR25519Keypair &keypair, gsl::span<uint8_t> message) const {
    SR25519Signature signature{};

    try {
      sr25519_sign(signature.data(),
                   keypair.public_key.data(),
                   keypair.secret_key.data(),
                   message.data(),
                   message.size());
    } catch (...) {
      return SR25519ProviderError::SIGN_UNKNOWN_ERROR;
    }

    return signature;
  }

  outcome::result<bool> SR25519ProviderImpl::verify(
      const SR25519Signature &signature,
      gsl::span<uint8_t> message,
      const SR25519PublicKey &public_key) const {
    bool result = false;
    try {
      result = sr25519_verify(
          signature.data(), message.data(), message.size(), public_key.data());
    } catch (...) {
      return SR25519ProviderError::SIGN_UNKNOWN_ERROR;
    }
    return outcome::success(result);
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, SR25519ProviderError, e) {
  using kagome::crypto::SR25519ProviderError;
  switch (e) {
    case SR25519ProviderError::SIGN_UNKNOWN_ERROR:
      return "failed to sign message, unknown error occured";
    case SR25519ProviderError::VERIFY_UNKNOWN_ERROR:
      return "failed to verify message, unknown error occured";
  }
  return "unknown SR25519ProviderError";
}
