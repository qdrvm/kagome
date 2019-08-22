/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNCHRONIZER_HPP
#define KAGOME_SYNCHRONIZER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "libp2p/peer/peer_id.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus {
  /**
   * Synchronizer, which retrieves blocks from other peers; mostly used to
   * *synchronize* this node's state with the state of other nodes, for example,
   * if this node does not have some blocks
   */
  struct Synchronizer {
    virtual ~Synchronizer() = default;

    /**
     * Start accepting requests for blocks
     */
    virtual void start() = 0;

    /**
     * Announce the network about a new block
     * @param block_header to be announced
     */
    virtual void announce(const primitives::BlockHeader &block_header) = 0;

    /**
     * Empty result, because methods, which use it, insert retrieved blocks
     * directly into the shared local tree
     */
    using RequestCallback = std::function<void(outcome::result<void>)>;

    /**
     * Request new blocks, starting from our deepest block; received blocks will
     * be inserted into the block tree
     * @param peer to request blocks from
     * @param cb to be called, when the operation finishes; contains nothing on
     * success and error otherwise
     *
     * @note this function will request the (\param peer) for all new blocks,
     * starting from our deepest one
     * @note example use-case: the node starts and downloads the chain from
     * one of the peers it knows
     */
    virtual void requestBlocks(const libp2p::peer::PeerId &peer,
                               RequestCallback cb) = 0;

    /**
     * Request new blocks
     * @param peer to request blocks from; most probably this is the peer, which
     * sent the block
     * @param hash of the block, which is to appear in the result of the request
     * @param cb to be called, when the operation finishes; contains nothing on
     * success and error otherwise
     *
     * @note this function will request all peers we know until the (\param
     * hash) block appears or we asked all known peers
     * @note example use-case: the node received a block, which "parent_hash" is
     * unknown to us; this method tries to download the lacking part to the
     * local tree
     */
    virtual void requestBlocks(const libp2p::peer::PeerId &peer,
                               const primitives::BlockHash &hash,
                               RequestCallback cb) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_SYNCHRONIZER_HPP
