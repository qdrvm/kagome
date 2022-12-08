/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGER
#define KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGER

#include <boost/optional.hpp>

#include "common/tagged.hpp"
#include "primitives/authority.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieBatch;
}  // namespace kagome::storage::trie

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::consensus::grandpa {

  using IsBlockFinalized = Tagged<bool, struct IsBlockFinalizedTag>;

  class AuthorityManager {
   public:
    virtual ~AuthorityManager() = default;

    /**
     * @return block associated with the root of scheduled changes tree
     */
    virtual primitives::BlockInfo base() const = 0;

    /**
     * @brief Returns authorities according specified block
     * @param block for which authority set is requested
     * @param finalized - true if we consider that the provided block is
     * finalized
     * @return outcome authority set
     */
    virtual std::optional<std::shared_ptr<const primitives::AuthoritySet>>
    authorities(const primitives::BlockInfo &block,
                IsBlockFinalized finalized) const = 0;

    /**
     * @brief Schedule an authority set change after the given delay of N
     * blocks, after next one would be finalized by the finality consensus
     * engine
     * @param context data about block where a digest with this change was
     * discovered
     * @param authorities is authority set for renewal
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyScheduledChange(
        const primitives::BlockContext &context,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) = 0;

    /**
     * @brief Force an authority set change after the given delay of N blocks,
     * after next one would be imported block which has been validated by the
     * block production consensus engine.
     * @param context data about block where a digest with this change was
     * discovered
     * @param authorities new authority set
     * @param delay_start block at which the delay before this change is applied
     * starts
     * @param delay the chain length until the delay is over
     */
    virtual outcome::result<void> applyForcedChange(
        const primitives::BlockContext &context,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber delay_start,
        size_t delay) = 0;

    /**
     * @brief An index of the individual authority in the current authority list
     * that should be immediately disabled until the next authority set change.
     * When an authority gets disabled, the node should stop performing any
     * authority functionality from that authority, including authoring blocks
     * and casting GRANDPA votes for finalization. Similarly, other nodes should
     * ignore all messages from the indicated authority which pertain to their
     * authority role. Once an authority set change after the given delay of N
     * blocks, is an imported block which has been validated by the block
     * production consensus engine.
     * @param context data about block where a digest with this change was
     * discovered
     * @param authority_index is index of one authority in current authority set
     */
    virtual outcome::result<void> applyOnDisabled(
        const primitives::BlockContext &context, uint64_t authority_index) = 0;

    /**
     * @brief A signal to pause the current authority set after the given delay,
     * is a block finalized by the finality consensus engine. After finalizing
     * block, the authorities should stop voting.
     * @param context data about block where a digest with this change was
     * discovered
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyPause(
        const primitives::BlockContext &context,
        primitives::BlockNumber activate_at) = 0;

    /**
     * @brief A signal to resume the current authority set after the given
     * delay, is an imported block and validated by the block production
     * consensus engine. After authoring the block B 0 , the authorities should
     * resume voting.
     * @param context data about block where a digest with this change was
     * discovered
     * @param activateAt is number of block when changes will applied
     */
    virtual outcome::result<void> applyResume(
        const primitives::BlockContext &context,
        primitives::BlockNumber activate_at) = 0;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGER
