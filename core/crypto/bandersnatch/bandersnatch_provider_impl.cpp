/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bandersnatch_provider_impl.hpp"

namespace kagome ::crypto {
  //
  BandersnatchKeypair BandersnatchProviderImpl::generateKeypair(
      const BandersnatchSeed &seed,
      BandersnatchProvider::Junctions junctions) const {
    return {};
  }

  outcome::result<BandersnatchSignature> BandersnatchProviderImpl::sign(
      const BandersnatchKeypair &keypair, common::BufferView message) const {
    return {{}};
  }

  outcome::result<bool> BandersnatchProviderImpl::verify(
      const BandersnatchSignature &signature,
      common::BufferView message,
      const BandersnatchPublicKey &public_key) const {
    return false;
  }
}  // namespace kagome::crypto
