/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"

namespace kagome::crypto {

  BandersnatchKeypair BandersnatchProviderImpl::generateKeypair(
      const BandersnatchSeed &seed,
      BandersnatchProvider::Junctions junctions) const {
    std::array<uint8_t, constants::bandersnatch::KEYPAIR_SIZE> kp{};

    bandersnatch_keypair_from_seed(seed.data(), kp.data());

    BandersnatchKeypair keypair;

    std::copy_n(kp.begin(),
                constants::bandersnatch::SECRET_SIZE,
                keypair.secret_key.begin());
    std::copy_n(kp.begin() + constants::bandersnatch::SECRET_SIZE,
                constants::bandersnatch::PUBLIC_SIZE,
                keypair.public_key.begin());

    return keypair;
  }

  outcome::result<BandersnatchSignature> BandersnatchProviderImpl::sign(
      const BandersnatchKeypair &keypair, common::BufferView message) const {
    auto seed = BandersnatchSeed::fromSpan(keypair.secret_key).value();

    BandersnatchSignature signature;

    ::bandersnatch_sign(
        seed.data(), message.data(), message.size(), signature.data());

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
