/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ed25519/ed25519_provider_impl.hpp"

namespace kagome::crypto {

  outcome::result<ED25519Keypair> ED25519ProviderImpl::generateKeypair() const {
    private_key_t private_key_low{};
    public_key_t public_key_low{};

    auto res = ed25519_create_keypair(&private_key_low, &public_key_low);
    if (res == 0) {
      return ED25519ProviderError::FAILED_GENERATE_KEYPAIR;
    }

    auto priv_span = gsl::make_span<uint8_t>(private_key_low.data,
                                             constants::ed25519::PRIVKEY_SIZE);

    auto pub_span = gsl::make_span<uint8_t>(public_key_low.data,
                                            constants::ed25519::PUBKEY_SIZE);

    auto &&private_key_high = ED25519PrivateKey::fromSpan(priv_span);
    if (!private_key_high) {
      BOOST_UNREACHABLE_RETURN(ED25519Keypair{});
    }

    auto &&public_key_high = ED25519PublicKey::fromSpan(pub_span);
    if (!public_key_high) {
      BOOST_UNREACHABLE_RETURN(ED25519Keypair{});
    }

    return ED25519Keypair{private_key_high.value(), public_key_high.value()};
  }

  ED25519Keypair ED25519ProviderImpl::generateKeypair(
      const ED25519Seed &seed) const {
    private_key_t private_key_low{};
    public_key_t public_key_low{};
    std::copy_n(seed.begin(), ED25519Seed::size(), private_key_low.data);

    // derive public key
    ed25519_derive_public_key(&private_key_low, &public_key_low);
    ED25519PrivateKey privateKey{seed};
    auto pub_span = gsl::make_span<uint8_t>(public_key_low.data,
                                            constants::ed25519::PUBKEY_SIZE);
    auto &&public_key_high = ED25519PublicKey::fromSpan(pub_span);
    if (!public_key_high) {
      BOOST_UNREACHABLE_RETURN(ED25519Keypair{});
    }

    return ED25519Keypair{privateKey, public_key_high.value()};
  }

  outcome::result<ED25519Signature> ED25519ProviderImpl::sign(
      const ED25519Keypair &keypair, gsl::span<uint8_t> message) const {
    signature_t signature_low{};
    public_key_t public_key{};
    private_key_t private_key{};
    auto private_span = gsl::make_span<uint8_t>(
        private_key.data, constants::ed25519::PRIVKEY_SIZE);
    auto public_span = gsl::make_span<uint8_t>(public_key.data,
                                               constants::ed25519::PUBKEY_SIZE);
    std::copy_n(keypair.private_key.begin(),
                constants::ed25519::PRIVKEY_SIZE,
                private_span.begin());
    std::copy_n(keypair.public_key.begin(),
                constants::ed25519::PUBKEY_SIZE,
                public_span.begin());

    try {
      ed25519_sign(&signature_low,
                   message.data(),
                   message.size(),
                   &public_key,
                   &private_key);
    } catch (...) {
      return ED25519ProviderError::SIGN_UNKNOWN_ERROR;
    }

    ED25519Signature signature{};
    auto signature_span = gsl::make_span<uint8_t>(
        signature_low.data, constants::ed25519::SIGNATURE_SIZE);
    std::copy_n(signature_span.begin(),
                constants::ed25519::SIGNATURE_SIZE,
                signature.begin());

    return signature;
  }

  outcome::result<bool> ED25519ProviderImpl::verify(
      const ED25519Signature &signature,
      gsl::span<uint8_t> message,
      const ED25519PublicKey &public_key) const {
    public_key_t public_key_low{};
    signature_t signature_low{};
    auto public_span = gsl::span<uint8_t>(public_key_low.data,
                                          constants::ed25519::PUBKEY_SIZE);
    auto signature_span = gsl::span<uint8_t>(
        signature_low.data, constants::ed25519::SIGNATURE_SIZE);

    std::copy_n(signature.data(),
                constants::ed25519::SIGNATURE_SIZE,
                signature_low.data);

    std::copy_n(public_key.data(),
                constants::ed25519::PUBKEY_SIZE,
                public_key_low.data);

    try {
      const auto res = ed25519_verify(
          &signature_low, message.data(), message.size(), &public_key_low);
      return res == ED25519_SUCCESS;
    } catch (...) {
      return ED25519ProviderError::VERIFY_UNKNOWN_ERROR;
    }
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, ED25519ProviderError, e) {
  using kagome::crypto::ED25519ProviderError;
  switch (e) {
    case ED25519ProviderError::FAILED_GENERATE_KEYPAIR:
      return "failed to generate keypair, maybe problems with random source";
    case ED25519ProviderError::SIGN_UNKNOWN_ERROR:
      return "failed to sign message, unknown error occured";
    case ED25519ProviderError::VERIFY_UNKNOWN_ERROR:
      return "faield to verify message, unknown error occured";
  }
  return "unknown SR25519ProviderError";
}
