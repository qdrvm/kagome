/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
