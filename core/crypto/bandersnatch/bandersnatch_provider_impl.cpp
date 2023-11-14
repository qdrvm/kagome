/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bandersnatch_provider_impl.hpp"

namespace kagome ::crypto {

  BandersnatchKeypair BandersnatchProviderImpl::generateKeypair(
      const BandersnatchSeed &seed,
      BandersnatchProvider::Junctions junctions) const {
    throw std::runtime_error(
        "Method 'BandersnatchProviderImpl::generateKeypair' is not implemented "
        "yet");
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
