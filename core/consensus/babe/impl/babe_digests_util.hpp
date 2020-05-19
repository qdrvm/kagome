/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_BABE_DIGESTS_UTIL_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_BABE_DIGESTS_UTIL_HPP

#include <boost/optional.hpp>

#include "common/visitor.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/next_epoch_descriptor.hpp"
#include "consensus/babe/types/seal.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {

  enum class DigestError {
    INVALID_DIGESTS = 1,
    MULTIPLE_EPOCH_CHANGE_DIGESTS,
    NEXT_EPOCH_DIGEST_DOES_NOT_EXIST
  };

  template <typename T, typename VarT>
  boost::optional<T> getFromVariant(VarT &&v) {
    return visit_in_place(
        v,
        [](const T &expected_val) -> boost::optional<T> {
          return boost::get<T>(expected_val);
        },
        [](const auto &) -> boost::optional<T> { return boost::none; });
  }

  outcome::result<std::pair<Seal, BabeBlockHeader>> getBabeDigests(
      const primitives::BlockHeader &header);

  outcome::result<NextEpochDescriptor> getNextEpochDigest(
      const primitives::BlockHeader &header);

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, DigestError)

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_BABE_DIGESTS_UTIL_HPP
