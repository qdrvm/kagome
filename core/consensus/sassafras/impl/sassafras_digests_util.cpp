/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras_digests_util.hpp"

#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::sassafras, DigestError, e) {
  using E = kagome::consensus::sassafras::DigestError;
  switch (e) {
    case E::REQUIRED_DIGESTS_NOT_FOUND:
      return "the block must contain at least BABE "
             "header and seal digests";
    case E::NO_TRAILING_SEAL_DIGEST:
      return "the block must contain a seal digest as the last digest";
    case E::GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS:
      return "genesis block can not have digests";
  }
  return "unknown error (kagome::consensus::sassafras::DigestError)";
}

namespace {
  template <typename T, typename VarT>
  std::optional<std::reference_wrapper<const std::decay_t<T>>> getFromVariant(
      VarT &&v) {
    return visit_in_place(
        std::forward<VarT>(v),
        [](const T &expected_val)
            -> std::optional<std::reference_wrapper<const std::decay_t<T>>> {
          return expected_val;
        },
        [](const auto &) { return std::nullopt; });
  }
}  // namespace

namespace kagome::consensus::sassafras {
  outcome::result<SlotNumber> getSlot(const primitives::BlockHeader &header) {
    OUTCOME_TRY(babe_block_header, getSassafrasBlockHeader(header));
    return babe_block_header.slot_number;
  }

  outcome::result<SassafrasBlockHeader> getSassafrasBlockHeader(
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
        auto sassafras_block_header_res =
            scale::decode<SassafrasBlockHeader>(pre_runtime_opt->get().data);
        if (sassafras_block_header_res.has_value()) {
          // found the SassafrasBlockHeader digest; return
          return sassafras_block_header_res.value();
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

    OUTCOME_TRY(seal_digest, scale::decode<Seal>(seal_opt->get().data));

    return seal_digest;
  }

}  // namespace kagome::consensus::sassafras
