/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "crypto/sr25519_types.hpp"
#include "libp2p/crypto/random_generator.hpp"

namespace kagome::crypto {
  Sr25519ProviderImpl::Sr25519ProviderImpl(std::shared_ptr<CSPRNG> generator)
      : generator_(std::move(generator)) {
    BOOST_ASSERT(generator_ != nullptr);
  }

  Sr25519KeypairAndSeed Sr25519ProviderImpl::generateKeypair() const {
    Sr25519Seed seed;
    generator_->fillRandomly(seed);
    return Sr25519KeypairAndSeed{generateKeypair(seed), seed};
  }

  Sr25519Keypair Sr25519ProviderImpl::generateKeypair(
      const Sr25519Seed &seed) const {
    std::array<uint8_t, constants::sr25519::KEYPAIR_SIZE> kp{};
    sr25519_keypair_from_seed(kp.data(), seed.data());

    Sr25519Keypair keypair;
    std::copy(kp.begin(),
              kp.begin() + constants::sr25519::SECRET_SIZE,
              keypair.secret_key.begin());
    std::copy(kp.begin() + constants::sr25519::SECRET_SIZE,
              kp.begin() + constants::sr25519::SECRET_SIZE
                  + constants::sr25519::PUBLIC_SIZE,
              keypair.public_key.begin());
    return keypair;
  }

  outcome::result<Sr25519Signature> Sr25519ProviderImpl::sign(
      const Sr25519Keypair &keypair, gsl::span<const uint8_t> message) const {
    Sr25519Signature signature{};

    try {
      sr25519_sign(signature.data(),
                   keypair.public_key.data(),
                   keypair.secret_key.data(),
                   message.data(),
                   message.size());
    } catch (...) {
      return Sr25519ProviderError::SIGN_UNKNOWN_ERROR;
    }

    return signature;
  }

  outcome::result<bool> Sr25519ProviderImpl::verify_deprecated(
      const Sr25519Signature &signature,
      gsl::span<const uint8_t> message,
      const Sr25519PublicKey &public_key) const {
    bool result = false;
    try {
      result = sr25519_verify_deprecated(
          signature.data(), message.data(), message.size(), public_key.data());
    } catch (...) {
      return Sr25519ProviderError::SIGN_UNKNOWN_ERROR;
    }
    return result;
  }

  outcome::result<bool> Sr25519ProviderImpl::verify(
      const Sr25519Signature &signature,
      gsl::span<const uint8_t> message,
      const Sr25519PublicKey &public_key) const {
    bool result = false;
    try {
      result = sr25519_verify(
          signature.data(), message.data(), message.size(), public_key.data());
    } catch (...) {
      return Sr25519ProviderError::SIGN_UNKNOWN_ERROR;
    }
    return outcome::success(result);
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, Sr25519ProviderError, e) {
  using kagome::crypto::Sr25519ProviderError;
  switch (e) {
    case Sr25519ProviderError::SIGN_UNKNOWN_ERROR:
      return "failed to sign message, unknown error occured";
    case Sr25519ProviderError::VERIFY_UNKNOWN_ERROR:
      return "failed to verify message, unknown error occured";
  }
  return "unknown Sr25519ProviderError";
}
