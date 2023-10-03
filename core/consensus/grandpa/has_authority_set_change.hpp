/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"

namespace kagome::consensus::grandpa {
  struct HasAuthoritySetChange {
    HasAuthoritySetChange(const primitives::BlockHeader &block) {
      for (auto &digest : block.digest) {
        auto consensus = boost::get<primitives::Consensus>(&digest);
        if (not consensus) {
          continue;
        }
        auto decoded_res = consensus->decode();
        if (not decoded_res) {
          continue;
        }
        auto &decoded = decoded_res.value();
        auto grandpa = boost::get<primitives::GrandpaDigest>(&decoded.digest);
        if (not grandpa) {
          continue;
        }
        if (auto change = boost::get<primitives::ScheduledChange>(grandpa)) {
          scheduled = std::move(*change);
          continue;
        }
        if (auto change = boost::get<primitives::ForcedChange>(grandpa)) {
          forced = std::move(*change);
          continue;
        }
      }
    }

    operator bool() const {
      return scheduled || forced;
    }

    std::optional<primitives::ScheduledChange> scheduled;
    std::optional<primitives::ForcedChange> forced;
  };
}  // namespace kagome::consensus::grandpa
