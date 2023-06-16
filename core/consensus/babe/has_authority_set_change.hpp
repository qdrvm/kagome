/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_HAS_AUTHORITY_SET_CHANGE_HPP
#define KAGOME_CONSENSUS_BABE_HAS_AUTHORITY_SET_CHANGE_HPP

#include "primitives/block_header.hpp"

namespace kagome::consensus::babe {
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
        auto babe = boost::get<primitives::BabeDigest>(&decoded.digest);
        if (not babe) {
          continue;
        }
        if (auto item = boost::get<primitives::NextEpochData>(babe)) {
          epoch = std::move(*item);
          continue;
        }
        if (auto item = boost::get<primitives::NextConfigData>(babe)) {
          config = boost::get<primitives::NextConfigDataV1>(*item);
          continue;
        }
      }
    }

    operator bool() const {
      return epoch.has_value();
    }

    std::optional<primitives::NextEpochData> epoch;
    std::optional<primitives::NextConfigDataV1> config;
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_HAS_AUTHORITY_SET_CHANGE_HPP
