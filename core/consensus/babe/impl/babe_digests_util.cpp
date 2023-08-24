/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
  }
  return "unknown error";
}

namespace kagome::consensus::babe {
  outcome::result<SlotNumber> getBabeSlot(
      const primitives::BlockHeader &header) {
    OUTCOME_TRY(digests, getBabeDigests(header));
    return digests.second.slot_number;
  }

  outcome::result<std::pair<Seal, BabeBlockHeader>> getBabeDigests(
      const primitives::BlockHeader &block_header) {
    // valid BABE block has at least two digests: BabeHeader and a seal
    if (block_header.digest.size() < 2) {
      return DigestError::REQUIRED_DIGESTS_NOT_FOUND;
    }
    const auto &digests = block_header.digest;

    // last digest of the block must be a seal - signature
    auto seal_opt = getFromVariant<primitives::Seal>(digests.back());
    if (not seal_opt.has_value()) {
      return DigestError::NO_TRAILING_SEAL_DIGEST;
    }

    OUTCOME_TRY(babe_seal_res, scale::decode<Seal>(seal_opt->get().data));

    for (const auto &digest :
         gsl::make_span(digests).subspan(0, digests.size() - 1)) {
      if (auto pre_runtime_opt = getFromVariant<primitives::PreRuntime>(digest);
          pre_runtime_opt.has_value()) {
        if (auto header =
                scale::decode<BabeBlockHeader>(pre_runtime_opt->get().data);
            header) {
          // found the BabeBlockHeader digest; return
          return {babe_seal_res, header.value()};
        }
      }
    }

    return DigestError::REQUIRED_DIGESTS_NOT_FOUND;
  }
}  // namespace kagome::consensus::babe
