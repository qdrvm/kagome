/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ed25519/ed25519_provider_impl.hpp"

#include <random>

extern "C" {
#include <schnorrkel/schnorrkel.h>
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, Ed25519ProviderImpl::Error, e) {
  using E = kagome::crypto::Ed25519ProviderImpl::Error;
  switch (e) {
    case E::VERIFICATION_FAILED:
      return "Internal error during ed25519 signature verification";
    case E::SIGN_FAILED:
      return "Internal error during ed25519 signing";
  }
  return "Unknown error in ed25519 provider";
}

namespace kagome::crypto {

  Ed25519ProviderImpl::Ed25519ProviderImpl(std::shared_ptr<CSPRNG> generator)
      : generator_{std::move(generator)},
        logger_{log::createLogger("Ed25519Provider", "ed25519")} {
    BOOST_ASSERT(generator_ != nullptr);
  }

  Ed25519KeypairAndSeed Ed25519ProviderImpl::generateKeypair() const {
    Ed25519Seed seed;
    generator_->fillRandomly(seed);
    return Ed25519KeypairAndSeed{generateKeypair(seed), seed};
  }

  Ed25519Keypair Ed25519ProviderImpl::generateKeypair(
      const Ed25519Seed &seed) const {
    std::array<uint8_t, ED25519_KEYPAIR_LENGTH> kp_bytes{};
    ed25519_keypair_from_seed(kp_bytes.data(), seed.data());
    Ed25519Keypair kp;
    std::copy_n(
        kp_bytes.begin(), ED25519_SECRET_KEY_LENGTH, kp.secret_key.begin());
    std::copy_n(kp_bytes.begin() + ED25519_SECRET_KEY_LENGTH,
                ED25519_PUBLIC_KEY_LENGTH,
                kp.public_key.begin());
    return kp;
  }

  outcome::result<Ed25519Signature> Ed25519ProviderImpl::sign(
      const Ed25519Keypair &keypair, gsl::span<const uint8_t> message) const {
    Ed25519Signature sig;
    std::array<uint8_t, ED25519_KEYPAIR_LENGTH> keypair_bytes;
    std::copy(keypair.secret_key.begin(),
              keypair.secret_key.end(),
              keypair_bytes.begin());
    std::copy(keypair.public_key.begin(),
              keypair.public_key.end(),
              keypair_bytes.begin() + ED25519_SECRET_KEY_LENGTH);
    auto res = ed25519_sign(
        sig.data(), keypair_bytes.data(), message.data(), message.size_bytes());
    if (res != ED25519_RESULT_OK) {
      logger_->error("Error during ed25519 sign; error code: {}", res);
      return Error::SIGN_FAILED;
    }
    return sig;
  }
  outcome::result<bool> Ed25519ProviderImpl::verify(
      const Ed25519Signature &signature,
      gsl::span<const uint8_t> message,
      const Ed25519PublicKey &public_key) const {
    auto res = ed25519_verify(signature.data(),
                              public_key.data(),
                              message.data(),
                              message.size_bytes());
    if (res == ED25519_RESULT_OK) {
      return true;
    }
    if (res == ED25519_RESULT_VERIFICATION_FAILED) {
      return false;
    }
    logger_->error("Error verifying a signature; error code: {}", res);
    return Error::VERIFICATION_FAILED;
  }
}  // namespace kagome::crypto
