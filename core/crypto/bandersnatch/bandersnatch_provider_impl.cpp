/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bandersnatch/bandersnatch.hpp"

namespace kagome::crypto {

  BandersnatchKeypair BandersnatchProviderImpl::generateKeypair(
      const BandersnatchSeed &seed,
      BandersnatchProvider::Junctions junctions) const {
    bandersnatch_vrfs::SecretKey secret{seed};

    // FIXME
    // for (auto &junction : junctions) {
    //   decltype(kp) next;
    //   (junction.hard ? sr25519_derive_keypair_hard
    //                  : sr25519_derive_keypair_soft)(
    //       next.data(), kp.data(), junction.cc.data());
    //   kp = next;
    // }

    std::array<uint8_t, constants::bandersnatch::KEYPAIR_SIZE> kp{};
    //    bandersnatch_keypair_from_seed(kp.data(), seed.data());

    BandersnatchKeypair keypair;
    keypair.public_key = common::Blob(secret.publicKey());

    std::copy(kp.begin(),
              kp.begin() + constants::bandersnatch::SECRET_SIZE,
              keypair.secret_key.begin());
    std::copy(kp.begin() + constants::bandersnatch::SECRET_SIZE,
              kp.begin() + constants::bandersnatch::SECRET_SIZE
                  + constants::bandersnatch::PUBLIC_SIZE,
              keypair.public_key.begin());
    return keypair;
  }

  outcome::result<BandersnatchSignature> BandersnatchProviderImpl::sign(
      const BandersnatchKeypair &keypair, common::BufferView message) const {
    throw std::runtime_error(
        "Method 'BandersnatchProviderImpl::sign' is not implemented yet");
  }

  outcome::result<bool> BandersnatchProviderImpl::verify(
      const BandersnatchSignature &signature,
      common::BufferView message,
      const BandersnatchPublicKey &public_key) const {
    throw std::runtime_error(
        "Method 'BandersnatchProviderImpl::verify' is not implemented yet");
  }

}  // namespace kagome::crypto
