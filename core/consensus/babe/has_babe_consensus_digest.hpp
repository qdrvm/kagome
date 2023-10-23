/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"

#include "log/logger.hpp"

namespace kagome::consensus::babe {
  struct HasBabeConsensusDigest {
    static auto logger() {
      return log::createLogger("HasBabeConsensusDigest", "babe");
    }

    HasBabeConsensusDigest(const primitives::BlockHeader &block) {
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
