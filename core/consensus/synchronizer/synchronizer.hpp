/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNCHRONIZER_HPP
#define KAGOME_SYNCHRONIZER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "libp2p/peer/peer_info.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  /**
   * Synchronizer is used by the consensus in three cases:
   *  - we produced a block and want to broadcast it over the network
   *  - the local peer does not have block, which it is supposed to have (for
   *    example, new block arrives, but its parent hash is unknown to it), so it
   *    can ask another peer for the blocks it lacks
   *  - another peer suffers for the same reasons and asks the local one to
   *    provide the blocks
   */
  struct Synchronizer {
    virtual ~Synchronizer() = default;

    /**
     * Announce the network about a new block
     * @param block to be announced
     */
    virtual void announce(const primitives::Block &block) = 0;

    using RequestCallback = std::function<outcome::result<void>()>;

    /**
     * Request new blocks, starting from our deepest block
     * @param peer to request blocks from
     * @param cb to be called, when the operation finishes
     *
     * @note this function will request the (\param peer) for all new blocks,
     * starting from our deepest one
     */
    virtual void requestBlocks(const libp2p::peer::PeerInfo &peer,
                               RequestCallback cb) = 0;

    /**
     * Request new blocks, starting from our deepest block;
     * @param hash of the block, which it to appear in the result of the request
     * @param cb to be called, when the operation finishes
     *
     * @note this function will request all peers we know until the (\param
     * hash) block appears or we asked all known peers
     */
    virtual void requestBlocks(const primitives::BlockHash &hash,
                               primitives::BlockNumber number,
                               RequestCallback cb) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_SYNCHRONIZER_HPP
