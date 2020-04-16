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

#ifndef KAGOME_CORE_RUNTIME_GRANDPA_HPP
#define KAGOME_CORE_RUNTIME_GRANDPA_HPP

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "primitives/authority.hpp"
#include "primitives/block_id.hpp"
#include "primitives/digest.hpp"
#include "primitives/scheduled_change.hpp"
#include "primitives/session_key.hpp"

namespace kagome::runtime {
  // https://github.com/paritytech/substrate/blob/8bf08ca63491961fafe6adf414a7411cb3953dcf/core/finality-grandpa/primitives/src/lib.rs#L56

  /**
   * @brief interface for Grandpa runtime functions
   */
  class Grandpa {
   protected:
    using Digest = primitives::Digest;
    using ScheduledChange = primitives::ScheduledChange;
    using BlockNumber = primitives::BlockNumber;
    using SessionKey = primitives::SessionKey;
    using WeightedAuthority = primitives::Authority;
    using ForcedChange = primitives::ForcedChange;
    using BlockId = primitives::BlockId;

   public:
    virtual ~Grandpa() = default;
    /**
     * @brief calls Grandpa_pending_change runtime api function,
     * which checks a digest for pending changes.
     * @param digest digest to check
     * @return nullopt if there are no pending changes,
     * scheduled change item if exists or error if error occured
     */
    virtual outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) = 0;

    /**
     * @brief calls Grandpa_forced_change runtime api function
     * which checks a digest for forced changes
     * @param digest digest to check
     * @return nullopt if no forced changes,
     * forced change item if exists or error if error occured
     *
     */
    virtual outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) = 0;

    /**
     * @brief calls Grandpa_authorities runtime api function
     * @return collection of current grandpa authorities with their weights
     */
    virtual outcome::result<std::vector<WeightedAuthority>> authorities(
        const primitives::BlockId &block_id) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_GRANDPA_HPP
