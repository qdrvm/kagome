/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_MANAGER
#define KAGOME_AUTHORITY_MANAGER

#include <boost/optional.hpp>

#include "primitives/authority.hpp"

namespace kagome::authority {
  class AuthorityManager {
   public:
    virtual ~AuthorityManager() = default;

    virtual primitives::BlockInfo base() const = 0;

    /**
     * @brief Returns authorities according specified block
     * @param block for which authority set is requested
     * @param finalized - true if we consider that the provided block is finalized
     * @return outcome authority set
     */
    virtual outcome::result<std::shared_ptr<const primitives::AuthorityList>>
    authorities(const primitives::BlockInfo &block, bool finalized) = 0;

    /**
     * @brief Schedule an authority set change after the given delay of N
     * blocks, after next one would be finalized by the finality consensus
     * engine
     * @param block is info of block which representing this change
     * @param authorities is authority set for renewal
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyScheduledChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) = 0;

    /**
     * @brief Force an authority set change after the given delay of N blocks,
     * after next one would be imported block which has been validated by the
     * block production conensus engine.
     * @param block is info of block which representing this change
     * @param authorities is authotity set for renewal
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyForcedChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) = 0;

    /**
     * @brief An index of the individual authority in the current authority list
     * that should be immediately disabled until the next authority set change.
     * When an authority gets disabled, the node should stop performing any
     * authority functionality from that authority, including authoring blocks
     * and casting GRANDPA votes for finalization. Similarly, other nodes should
     * ignore all messages from the indicated authority which pretain to their
     * authority role.orce an authority set change after the given delay of N
     * blocks, is an imported block which has been validated by the block
     * production conensus engine.
     * @param block is info of block which representing this change
     * @param authority_index is index of one authority in current authority set
     */
    virtual outcome::result<void> applyOnDisabled(
        const primitives::BlockInfo &block, uint64_t authority_index) = 0;

    /**
     * @brief A signal to pause the current authority set after the given delay,
     * is a block finalized by the finality consensus engine. After finalizing
     * block, the authorities should stop voting.
     * @param block is info of block which representing this change
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyPause(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) = 0;

    /**
     * @brief A signal to resume the current authority set after the given
     * delay, is an imported block and validated by the block production
     * consensus engine. After authoring the block B 0 , the authorities should
     * resume voting.
     * @param block is info of block which representing this change
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyResume(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) = 0;

    /**
     * @brief Prunes data which was needed only till {@param block}
     * and won't be used anymore
     */
    virtual outcome::result<void> prune(const primitives::BlockInfo &block) = 0;
  };
}  // namespace kagome::authority

#endif  // KAGOME_AUTHORITY_MANAGER
