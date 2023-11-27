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
    case E::WRONG_ENGINE_ID:
      return "expected digest with engine id 'SASS'";
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
    OUTCOME_TRY(slot_claim, getSlotClaim(header));
    return slot_claim.slot_number;
  }

  outcome::result<SlotClaim> getSlotClaim(
      const primitives::BlockHeader &block_header) {
    [[unlikely]] if (block_header.number == 0) {
      return DigestError::GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS;
    }

    if (block_header.digest.empty()) {
      return DigestError::REQUIRED_DIGESTS_NOT_FOUND;
    }
    const auto &digests = block_header.digest;

    for (const auto &digest : std::span(digests).first(digests.size() - 1)) {
      auto pre_runtime_opt = getFromVariant<primitives::PreRuntime>(digest);
      if (not pre_runtime_opt.has_value()) {
        continue;
      }

      const auto &engine_id = pre_runtime_opt->get().consensus_engine_id;
      if (engine_id != primitives::kSassafrasEngineId) {
        continue;
      }

      const auto &data = pre_runtime_opt->get().data;
      auto slot_claim_res = scale::decode<SlotClaim>(data);
      if (slot_claim_res.has_value()) {
        // found the SlotClaim digest; return
        return slot_claim_res.value();
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

    // the last digest of the block must be a seal - signature
    auto seal_opt = getFromVariant<primitives::Seal>(digests.back());
    if (not seal_opt.has_value()) {
      return DigestError::NO_TRAILING_SEAL_DIGEST;
    }

    const auto &engine_id = seal_opt->get().consensus_engine_id;
    if (engine_id != primitives::kBabeEngineId) {
      return DigestError::WRONG_ENGINE_ID;
    }

    const auto &data = seal_opt->get().data;

    OUTCOME_TRY(seal_digest, scale::decode<Seal>(data));

    return seal_digest;
  }

}  // namespace kagome::consensus::sassafras
