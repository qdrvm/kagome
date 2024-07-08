/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"

namespace kagome::crypto {

  outcome::result<BandersnatchKeypair>
  BandersnatchProviderImpl::generateKeypair(
      const BandersnatchSeed &seed,
      BandersnatchProvider::Junctions junctions) const {
    std::array<uint8_t, constants::bandersnatch::KEYPAIR_SIZE> kp{};
    bandersnatch_keypair_from_seed(seed.unsafeBytes().data(), kp.data());

    BOOST_ASSERT_MSG(junctions.empty(), "Junctions is not supported now");

    BandersnatchKeypair keypair{
        BandersnatchSecretKey::from(SecureCleanGuard{
            std::span(kp).subspan<0, constants::bandersnatch::SECRET_SIZE>()}),
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
