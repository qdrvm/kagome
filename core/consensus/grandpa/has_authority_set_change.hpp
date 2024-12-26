/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"

namespace kagome::consensus::grandpa {
  struct HasAuthoritySetChange {
    HasAuthoritySetChange(const primitives::BlockHeader &block) {
      for (auto &digest : block.digest) {
        auto consensus = std::get_if<primitives::Consensus>(&digest);
        if (not consensus) {
          continue;
        }
        auto decoded_res = consensus->decode();
        if (not decoded_res) {
          continue;
        }
        auto &decoded = decoded_res.value();
        auto grandpa = std::get_if<primitives::GrandpaDigest>(&decoded.digest);
        if (not grandpa) {
          continue;
        }
        if (auto change = std::get_if<ScheduledChange>(grandpa)) {
          scheduled = std::move(*change);
          continue;
        }
        if (auto change = std::get_if<ForcedChange>(grandpa)) {
          forced = std::move(*change);
          continue;
        }
      }
    }

    operator bool() const {
      return scheduled || forced;
    }

    std::optional<ScheduledChange> scheduled;
    std::optional<ForcedChange> forced;
  };
}  // namespace kagome::consensus::grandpa
