/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/structs.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"

namespace kagome::consensus::grandpa {
  /**
   * Grandpa justifications lack authority set id.
   * Guess authority set id by brute force.
   * Upper bound is block number.
   */
  inline std::optional<AuthoritySetId> guessSetId(
      const GrandpaJustification &j) {
    static crypto::Ed25519ProviderImpl ed25519{nullptr};
    auto &vote = j.items.at(0);
    for (AuthoritySetId set = 0; set <= j.block_info.number; ++set) {
      auto m =
          scale::encode(std::tie(vote.message, j.round_number, set)).value();
      auto ok_res = ed25519.verify(vote.signature, m, vote.id);
      if (not ok_res.has_value()) {
        return std::nullopt;
      }
      if (ok_res.value()) {
        return set;
      }
    }
    return std::nullopt;
  }
}  // namespace kagome::consensus::grandpa
