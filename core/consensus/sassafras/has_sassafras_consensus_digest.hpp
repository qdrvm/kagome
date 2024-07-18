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
        auto consensus = std::get_if<primitives::Consensus>(&digest);
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
            std::get_if<primitives::SassafrasDigest>(&decoded.digest);
        if (not sassafras) {
          continue;
        }
        if (auto item = std::get_if<NextEpochDescriptor>(sassafras)) {
          descriptor = std::move(*item);
          continue;
        }
      }
    }

    operator bool() const {
      return descriptor.has_value();
    }

    std::optional<NextEpochDescriptor> descriptor;
  };
}  // namespace kagome::consensus::sassafras
