/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_GRANDPA_HPP
#define KAGOME_CORE_RUNTIME_GRANDPA_HPP

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
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
    using WeightedAuthority = primitives::WeightedAuthority;
    using ForcedChangeType = primitives::ForcedChangeType;
    using BlockId = primitives::BlockId;

   public:
    virtual ~Grandpa() = default;
    /**
     * @brief calls Grandpa_pending_change runtime api function
     * @param block_id block id
     * @param digest digest
     * @return optional ScheduledChange or error
     */
    virtual outcome::result<std::optional<ScheduledChange>> pendingChange(
        BlockId block_id, const Digest &digest) = 0;

    /**
     * @brief calls Grandpa_forced_change runtime api function
     * @param block_id block id
     * @param digest digest
     * @return don't really know the meaning of result
     */
    virtual outcome::result<std::optional<ForcedChangeType>> forcedChange(
        BlockId block_id, const Digest &digest) = 0;

    /**
     * @brief calls Grandpa_authorities runtime api function
     * @param block_id block id
     * @return collection of authorities with their weights
     */
    virtual outcome::result<std::vector<WeightedAuthority>> authorities(
        BlockId block_id) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_GRANDPA_HPP
