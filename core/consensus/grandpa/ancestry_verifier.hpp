/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "crypto/hasher.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus::grandpa {
  /**
   * Verifies ancestry merke proof from GrandpaJustification
   */
  struct AncestryVerifier {
    AncestryVerifier(const std::vector<primitives::BlockHeader> &blocks,
                     const crypto::Hasher &hasher) {
      for (auto &block : blocks) {
        if (block.number == 0) {
          continue;
        }
        // Calculate and save hash, 'cause data
        // has likely just received from network
        if (not block.hash_opt.has_value()) {
          primitives::calculateBlockHash(
              const_cast<primitives::BlockHeader &>(block), hasher);
        }
        parents.emplace(block.blockInfo(), *block.parentInfo());
      }
    }

    bool hasAncestry(const primitives::BlockInfo &ancestor,
                     const primitives::BlockInfo &descendant) const {
      auto block = descendant;
      while (block != ancestor) {
        if (block.number < ancestor.number) {
          return false;
        }
        auto it = parents.find(block);
        if (it == parents.end()) {
          return false;
        }
        block = it->second;
      }
      return true;
    }

    std::unordered_map<primitives::BlockInfo, primitives::BlockInfo> parents;
  };
}  // namespace kagome::consensus::grandpa
