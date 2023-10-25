/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"

#include "log/logger.hpp"

namespace kagome::consensus::sassafras {
  struct HasSassafrasConsensusDigest {
    static auto logger() {
      return log::createLogger("HasSassafrasConsensusDigest", "sassafras");
    }

    HasSassafrasConsensusDigest(const primitives::BlockHeader &block) {
      for (auto &digest : block.digest) {
        auto consensus = boost::get<primitives::Consensus>(&digest);
        if (not consensus) {
          continue;
        }
        auto decoded_res = consensus->decode();
        if (not decoded_res) {
          SL_WARN(logger(),
                  "error decoding digest block={} engine={} digest={}: {}",
                  block.number,
                  consensus->consensus_engine_id.toHex(),
                  consensus->data.toHex(),
                  decoded_res.error());
          continue;
        }
        auto &decoded = decoded_res.value();
        auto sassafras =
            boost::get<primitives::SassafrasDigest>(&decoded.digest);
        if (not sassafras) {
          continue;
        }
        if (auto item = boost::get<primitives::NextEpochData>(sassafras)) {
          epoch = std::move(*item);
          continue;
        }
        // if (auto item = boost::get<primitives::NextConfigData>(sassafras)) {
        //   config = boost::get<primitives::NextConfigDataV2>(*item);
        //   continue;
        // }
      }
    }

    operator bool() const {
      return epoch.has_value();
    }

    std::optional<primitives::NextEpochData> epoch;
    std::optional<primitives::NextConfigDataV2> config;
  };
}  // namespace kagome::consensus::sassafras
