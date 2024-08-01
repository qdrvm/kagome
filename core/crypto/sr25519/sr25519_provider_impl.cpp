/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "crypto/sr25519_types.hpp"
#include "utils/non_null_dangling.hpp"

namespace kagome::crypto {
  outcome::result<Sr25519Keypair> Sr25519ProviderImpl::generateKeypair(
      const Sr25519Seed &seed, Junctions junctions) const {
    std::array<uint8_t, constants::sr25519::KEYPAIR_SIZE> kp{};
    sr25519_keypair_from_seed(kp.data(), seed.unsafeBytes().data());
    for (auto &junction : junctions) {
      decltype(kp) next;
      (junction.hard ? sr25519_derive_keypair_hard
                     : sr25519_derive_keypair_soft)(
          next.data(), kp.data(), junction.cc.data());
      kp = next;
    }

    Sr25519Keypair keypair{
        Sr25519SecretKey::from(SecureCleanGuard{
            std::span(kp).subspan<0, constants::sr25519::SECRET_SIZE>()}),
        Sr25519PublicKey::fromSpan(
            std::span(kp).subspan(constants::sr25519::SECRET_SIZE,
                                  constants::sr25519::PUBLIC_SIZE))
            .value()};
    return keypair;
  }

  outcome::result<Sr25519Signature> Sr25519ProviderImpl::sign(
      const Sr25519Keypair &keypair, common::BufferView message) const {
    Sr25519Signature signature{};

    try {
      sr25519_sign(signature.data(),
                   keypair.public_key.data(),
                   keypair.secret_key.unsafeBytes().data(),
                   nonNullDangling(message),
                   message.size());
    } catch (...) {
      return Sr25519ProviderError::SIGN_UNKNOWN_ERROR;
    }

    return signature;
  }

  outcome::result<bool> Sr25519ProviderImpl::verify_deprecated(
      const Sr25519Signature &signature,
      common::BufferView message,
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
      common::BufferView message,
      const Sr25519PublicKey &public_key) const {
    bool result = false;
    try {
      result = sr25519_verify(signature.data(),
                              nonNullDangling(message),
                              message.size(),
                              public_key.data());
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
