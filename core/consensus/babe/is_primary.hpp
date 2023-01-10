/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_IS_PRIMARY_HPP
#define KAGOME_CONSENSUS_BABE_IS_PRIMARY_HPP

#include "consensus/babe/types/babe_block_header.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus::babe {
  inline bool isPrimary(const primitives::BlockHeader &block) {
    if (block.number == 0) {
      return false;
    }
    for (auto &digest : block.digest) {
      auto pre = boost::get<primitives::PreRuntime>(&digest);
      if (!pre) {
        continue;
      }
      if (pre->consensus_engine_id != primitives::kBabeEngineId) {
        continue;
      }
      auto _block = scale::decode<BabeBlockHeader>(pre->data);
      if (!_block) {
        continue;
      }
      return _block.value().slot_assignment_type == SlotType::Primary;
    }
    return false;
  }
}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_IS_PRIMARY_HPP
