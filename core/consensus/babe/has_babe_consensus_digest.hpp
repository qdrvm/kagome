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
        auto consensus = std::get_if<primitives::Consensus>(&digest);
        if (not consensus) {
          continue;
        }
        auto decoded_res = consensus->decodeConsensusMessage();
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
        auto babe = std::get_if<primitives::BabeDigest>(&decoded.digest);
        if (not babe) {
          continue;
        }
        if (auto item = std::get_if<EpochData>(babe)) {
          epoch = std::move(*item);
          continue;
        }
        if (auto item = std::get_if<NextConfigData>(babe)) {
          config = std::get<NextConfigDataV1>(*item);
          continue;
        }
      }
    }

    operator bool() const {
      return epoch.has_value();
    }

    std::optional<EpochData> epoch;
    std::optional<NextConfigDataV1> config;
  };
}  // namespace kagome::consensus::babe
