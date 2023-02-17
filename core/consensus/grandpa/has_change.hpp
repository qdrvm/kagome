/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_HAS_CHANGE_HPP
#define KAGOME_CONSENSUS_GRANDPA_HAS_CHANGE_HPP

#include "primitives/block_header.hpp"

namespace kagome::consensus::grandpa {
  struct HasChange {
    HasChange(const primitives::BlockHeader &block) {
      for (auto &digest : block.digest) {
        auto digest2 = boost::get<primitives::Consensus>(&digest);
        if (not digest2) {
          continue;
        }
        auto digest3 = digest2->decode();
        if (not digest3) {
          continue;
        }
        auto &digest4 = digest3.value();
        if (digest4.isGrandpaDigestOf<primitives::ScheduledChange>()) {
          scheduled = true;
        }
        if (digest4.isGrandpaDigestOf<primitives::ForcedChange>()) {
          forced = true;
        }
      }
    }

    operator bool() const {
      return scheduled || forced;
    }

    bool scheduled = false;
    bool forced = false;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_HAS_CHANGE_HPP
