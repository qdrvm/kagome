/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"

#include "crypto/hasher.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, BandersnatchProviderError, e) {
  using E = kagome::crypto::BandersnatchProviderError;
  switch (e) {
    case E::SIGN_UNKNOWN_ERROR:
      return "Internal error during bandersnatch signing";
    case E::VERIFY_UNKNOWN_ERROR:
      return "Internal error during bandersnatch signature verification";
    case E::SOFT_JUNCTION_NOT_SUPPORTED:
      return "Soft junction not supported for bandersnatch";
  }
  return "Unknown error (BandersnatchProviderError)";
}

namespace kagome::crypto {

  BandersnatchProviderImpl::BandersnatchProviderImpl(
      std::shared_ptr<Hasher> hasher)
      : hasher_(std::move(hasher)) {
    BOOST_ASSERT(hasher_);
  }

  outcome::result<BandersnatchKeypair>
  BandersnatchProviderImpl::generateKeypair(
      const BandersnatchSeed &seed,
      BandersnatchProvider::Junctions junctions) const {
    std::array<uint8_t, constants::bandersnatch::KEYPAIR_SIZE> kp{};

    if (junctions.empty()) {
      bandersnatch_keypair_from_seed(seed.unsafeBytes().data(), kp.data());
    } else {
      BandersnatchSeed seed_with_junctions = seed;
      for (auto &junction : junctions) {
        if (not junction.hard) {
          return BandersnatchProviderError::SOFT_JUNCTION_NOT_SUPPORTED;
        }
        auto hash = hasher_->blake2b_256(
            scale::encode(std::tuple("bandersnatch-vrf-HDKD"_bytes,
                                     seed_with_junctions.unsafeBytes(),
                                     junction.cc))
                .value());
        seed_with_junctions = BandersnatchSeed::from(SecureCleanGuard(hash));
      }
      bandersnatch_keypair_from_seed(seed_with_junctions.unsafeBytes().data(),
                                     kp.data());
    }

    BandersnatchKeypair keypair{
        .secret_key = BandersnatchSecretKey::from(SecureCleanGuard{
            std::span(kp).subspan<0, constants::bandersnatch::SECRET_SIZE>()}),
        .public_key =
            BandersnatchPublicKey::fromSpan(
                std::span(kp).subspan(constants::bandersnatch::SECRET_SIZE,
                                      constants::bandersnatch::PUBLIC_SIZE))
                .value()};
    return keypair;
  }

  outcome::result<BandersnatchSignature> BandersnatchProviderImpl::sign(
      const BandersnatchKeypair &keypair, common::BufferView message) const {
    BandersnatchSignature signature;

    ::bandersnatch_sign(keypair.secret_key.unsafeBytes().data(),
                        message.data(),
                        message.size(),
                        signature.data());

    return signature;
  }

  outcome::result<bool> BandersnatchProviderImpl::verify(
      const BandersnatchSignature &signature,
      common::BufferView message,
      const BandersnatchPublicKey &public_key) const {
    return ::bandersnatch_verify(
        signature.data(), message.data(), message.size(), public_key.data());
  }

}  // namespace kagome::crypto
