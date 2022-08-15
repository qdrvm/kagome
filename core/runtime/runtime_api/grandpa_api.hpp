/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_GRANDPAAPI
#define KAGOME_RUNTIME_GRANDPAAPI

#include <optional>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/authority.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"
#include "primitives/scheduled_change.hpp"
#include "primitives/session_key.hpp"

namespace kagome::runtime {
  // https://github.com/paritytech/substrate/blob/8bf08ca63491961fafe6adf414a7411cb3953dcf/core/finality-grandpa/primitives/src/lib.rs#L56

  /**
   * @brief interface for Grandpa runtime functions
   */
  class GrandpaApi {
   protected:
    using Digest = primitives::Digest;
    using ScheduledChange = primitives::ScheduledChange;
    using BlockNumber = primitives::BlockNumber;
    using AuthorityList = primitives::AuthorityList;
    using ForcedChange = primitives::ForcedChange;
    using BlockId = primitives::BlockId;

   public:
    virtual ~GrandpaApi() = default;
    /**
     * @brief calls Grandpa_pending_change runtime api function,
     * which checks a digest for pending changes.
     * @param digest digest to check
     * @return nullopt if there are no pending changes,
     * scheduled change item if exists or error if error occured
     */
    virtual outcome::result<std::optional<ScheduledChange>> pending_change(
        primitives::BlockHash const &block, const Digest &digest) = 0;

    /**
     * @brief calls Grandpa_forced_change runtime api function
     * which checks a digest for forced changes
     * @param digest digest to check
     * @return nullopt if no forced changes,
     * forced change item if exists or error if error occured
     *
     */
    virtual outcome::result<std::optional<ForcedChange>> forced_change(
        primitives::BlockHash const &block, const Digest &digest) = 0;

    /**
     * @brief calls Grandpa_authorities runtime api function
     * @return collection of current grandpa authorities with their weights
     */
    virtual outcome::result<AuthorityList> authorities(
        const primitives::BlockId &block_id) = 0;

    /**
     * @return the id of the current voter set at the provided block
     */
    virtual outcome::result<primitives::AuthoritySetId> current_set_id(
        const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_GRANDPAAPI
