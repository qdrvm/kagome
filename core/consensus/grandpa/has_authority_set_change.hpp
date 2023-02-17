/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_HAS_AUTHORITY_SET_CHANGE_HPP
#define KAGOME_CONSENSUS_GRANDPA_HAS_AUTHORITY_SET_CHANGE_HPP

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
        if (decoded.isGrandpaDigestOf<primitives::ScheduledChange>()) {
          scheduled = true;
        }
        if (decoded.isGrandpaDigestOf<primitives::ForcedChange>()) {
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

#endif  // KAGOME_CONSENSUS_GRANDPA_HAS_AUTHORITY_SET_CHANGE_HPP
