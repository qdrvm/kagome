/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <optional>

#include "common/visitor.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "consensus/timeline/types.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus::babe {

  enum class DigestError {
    REQUIRED_DIGESTS_NOT_FOUND = 1,
    NO_TRAILING_SEAL_DIGEST,
    GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS,
  };

  outcome::result<SlotNumber> getSlot(const primitives::BlockHeader &header);

  outcome::result<AuthorityIndex> getAuthority(
      const primitives::BlockHeader &header);

  outcome::result<BabeBlockHeader> getBabeBlockHeader(
      const primitives::BlockHeader &block_header);

  outcome::result<Seal> getSeal(const primitives::BlockHeader &block_header);

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, DigestError)
