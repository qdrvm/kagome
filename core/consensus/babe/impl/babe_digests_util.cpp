/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_digests_util.hpp"

#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe, DigestError, e) {
  using E = kagome::consensus::babe::DigestError;
  switch (e) {
    case E::REQUIRED_DIGESTS_NOT_FOUND:
      return "the block must contain at least BABE "
             "header and seal digests";
    case E::NO_TRAILING_SEAL_DIGEST:
      return "the block must contain a seal digest as the last digest";
    case E::GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS:
      return "genesis block can not have digests";
  }
  return "unknown error (kagome::consensus::babe::DigestError)";
}

namespace kagome::consensus::babe {
  outcome::result<SlotNumber> getBabeSlot(
      const primitives::BlockHeader &header) {
    OUTCOME_TRY(babe_block_header, getBabeBlockHeader(header));
    return babe_block_header.slot_number;
  }

  outcome::result<BabeBlockHeader> getBabeBlockHeader(
      const primitives::BlockHeader &block_header) {
    [[unlikely]] if (block_header.number == 0) {
      return DigestError::GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS;
    }

    if (block_header.digest.empty()) {
      return DigestError::REQUIRED_DIGESTS_NOT_FOUND;
    }
    const auto &digests = block_header.digest;

    for (const auto &digest :
         std::span(digests).subspan(0, digests.size() - 1)) {
      auto pre_runtime_opt = getFromVariant<primitives::PreRuntime>(digest);
      if (pre_runtime_opt.has_value()) {
        auto babe_block_header_res =
            scale::decode<BabeBlockHeader>(pre_runtime_opt->get().data.view());
        if (babe_block_header_res.has_value()) {
          // found the BabeBlockHeader digest; return
          return babe_block_header_res.value();
        }
      }
    }

    return DigestError::REQUIRED_DIGESTS_NOT_FOUND;
  }

  outcome::result<Seal> getSeal(const primitives::BlockHeader &block_header) {
    [[unlikely]] if (block_header.number == 0) {
      return DigestError::GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS;
    }

    if (block_header.digest.empty()) {
      return DigestError::REQUIRED_DIGESTS_NOT_FOUND;
    }
    const auto &digests = block_header.digest;

    // last digest of the block must be a seal - signature
    auto seal_opt = getFromVariant<primitives::Seal>(digests.back());
    if (not seal_opt.has_value()) {
      return DigestError::NO_TRAILING_SEAL_DIGEST;
    }

    OUTCOME_TRY(seal_digest, scale::decode<Seal>(seal_opt->get().data.view()));

    return seal_digest;
  }

}  // namespace kagome::consensus::babe
