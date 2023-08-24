/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_BABE_DIGESTS_UTIL_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_BABE_DIGESTS_UTIL_HPP

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
  };

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

  outcome::result<SlotNumber> getBabeSlot(
      const primitives::BlockHeader &header);

  outcome::result<std::pair<Seal, BabeBlockHeader>> getBabeDigests(
      const primitives::BlockHeader &header);
}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, DigestError)

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_BABE_DIGESTS_UTIL_HPP
