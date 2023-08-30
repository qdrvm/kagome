/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "crypto/ecdsa_provider.hpp"
#include "crypto/hasher/hasher_impl.hpp"

namespace kagome::consensus::beefy {
  inline common::Hash256 prehash(const Commitment &commitment) {
    // TODO(turuslan): #1768, scale encode to hash
    return crypto::HasherImpl{}.keccak_256(scale::encode(commitment).value());
  }

  inline bool verify(const crypto::EcdsaProvider &ecdsa,
                     const VoteMessage &vote) {
    auto r = ecdsa.verifyPrehashed(
        prehash(vote.commitment), vote.signature, vote.id);
    return r and r.value();
  }

  inline size_t threshold(size_t n) {
    return n == 0 ? 0 : n - (n - 1) / 3;
  }

  inline bool verify(const crypto::EcdsaProvider &ecdsa,
                     const BeefyJustification &justification_v1,
                     const ValidatorSet &validators) {
    auto &justification = boost::get<SignedCommitment>(justification_v1);
    if (justification.commitment.validator_set_id != validators.id) {
      return false;
    }
    auto total = validators.validators.size();
    if (justification.signatures.size() != total) {
      return false;
    }
    auto prehashed = prehash(justification.commitment);
    size_t valid = 0;
    for (size_t i = 0; i < total; ++i) {
      if (auto &sig = justification.signatures[i]) {
        if (auto r = ecdsa.verifyPrehashed(
                prehashed, *sig, validators.validators[i]);
            r and r.value()) {
          ++valid;
        }
      }
    }
    return valid >= threshold(total);
  }
}  // namespace kagome::consensus::beefy
