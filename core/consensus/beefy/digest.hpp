/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "primitives/block_header.hpp"

namespace kagome {
  inline std::optional<consensus::beefy::ValidatorSet> beefyValidatorsDigest(
      const primitives::BlockHeader &block) {
    for (auto &digest : block.digest) {
      auto consensus = boost::get<primitives::Consensus>(&digest);
      if (not consensus) {
        continue;
      }
      if (consensus->consensus_engine_id != primitives::kBeefyEngineId) {
        continue;
      }
      auto decoded_res =
          scale::decode<consensus::beefy::ConsensusDigest>(consensus->data);
      if (not decoded_res) {
        continue;
      }
      auto &decoded = decoded_res.value();
      if (auto item = boost::get<consensus::beefy::ValidatorSet>(&decoded)) {
        return std::move(*item);
      }
    }
    return std::nullopt;
  }

  inline std::optional<consensus::beefy::MmrRootHash> beefyMmrDigest(
      const primitives::BlockHeader &block) {
    for (auto &digest : block.digest) {
      auto consensus = boost::get<primitives::Consensus>(&digest);
      if (not consensus) {
        continue;
      }
      if (consensus->consensus_engine_id != primitives::kBeefyEngineId) {
        continue;
      }
      auto decoded_res =
          scale::decode<consensus::beefy::ConsensusDigest>(consensus->data);
      if (not decoded_res) {
        continue;
      }
      auto &decoded = decoded_res.value();
      if (auto item = boost::get<consensus::beefy::MmrRootHash>(&decoded)) {
        return *item;
      }
    }
    return std::nullopt;
  }
}  // namespace kagome
