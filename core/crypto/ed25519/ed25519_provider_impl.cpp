/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ed25519/ed25519_provider_impl.hpp"

extern "C" {
#include <schnorrkel/schnorrkel.h>
}

#include "crypto/common.hpp"
#include "crypto/hasher.hpp"
#include "scale/kagome_scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, Ed25519ProviderImpl::Error, e) {
  using E = kagome::crypto::Ed25519ProviderImpl::Error;
  switch (e) {
    case E::VERIFICATION_FAILED:
      return "Internal error during ed25519 signature verification";
    case E::SIGN_FAILED:
      return "Internal error during ed25519 signing";
    case E::SOFT_JUNCTION_NOT_SUPPORTED:
      return "Soft junction not supported for ed25519";
  }
  return "Unknown error in ed25519 provider";
}

namespace kagome::crypto {
  Ed25519ProviderImpl::Ed25519ProviderImpl(
      std::shared_ptr<crypto::Hasher> hasher)
      : hasher_{std::move(hasher)},
        logger_{log::createLogger("Ed25519Provider", "ed25519")} {}

  outcome::result<Ed25519Keypair> Ed25519ProviderImpl::generateKeypair(
      const Ed25519Seed &_seed, Junctions junctions) const {
    auto seed = _seed;
    for (auto &junction : junctions) {
      if (not junction.hard) {
        return Error::SOFT_JUNCTION_NOT_SUPPORTED;
      }
      auto hash = hasher_->blake2b_256(
          scale::encode(
              std::tuple("Ed25519HDKD"_bytes, seed.unsafeBytes(), junction.cc))
              .value());
      seed = Ed25519Seed::from(SecureCleanGuard(hash));
    }
    std::array<uint8_t, ED25519_KEYPAIR_LENGTH> kp_bytes{};
    ed25519_keypair_from_seed(kp_bytes.data(), seed.unsafeBytes().data());
    Ed25519Keypair kp{
        .secret_key = Ed25519PrivateKey::from(SecureCleanGuard{
            std::span(kp_bytes).subspan<0, ED25519_SECRET_KEY_LENGTH>()}),
        .public_key = Ed25519PublicKey::fromSpan(
                          std::span(kp_bytes)
                              .subspan<ED25519_SECRET_KEY_LENGTH,
                                       ED25519_PUBLIC_KEY_LENGTH>())
                          .value()};
    return kp;
  }

  outcome::result<Ed25519Signature> Ed25519ProviderImpl::sign(
      const Ed25519Keypair &keypair, common::BufferView message) const {
    Ed25519Signature sig;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    std::array<uint8_t, ED25519_KEYPAIR_LENGTH> keypair_bytes;
    SecureCleanGuard g{keypair_bytes};
    std::ranges::copy(keypair.secret_key.unsafeBytes(), keypair_bytes.begin());
    std::ranges::copy(keypair.public_key,
                      keypair_bytes.begin() + ED25519_SECRET_KEY_LENGTH);
    auto res = ed25519_sign(
        sig.data(), keypair_bytes.data(), message.data(), message.size_bytes());
    if (res != ED25519_RESULT_OK) {
      SL_ERROR(logger_,
               "Error during ed25519 sign; error code: {}",
               static_cast<size_t>(res));
      return Error::SIGN_FAILED;
    }
    return sig;
  }
  outcome::result<bool> Ed25519ProviderImpl::verify(
      const Ed25519Signature &signature,
      common::BufferView message,
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
    SL_ERROR(logger_,
             "Error verifying a signature; error code: {}",
             static_cast<size_t>(res));
    return Error::VERIFICATION_FAILED;
  }
}  // namespace kagome::crypto
