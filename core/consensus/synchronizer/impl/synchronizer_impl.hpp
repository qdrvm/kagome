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

#ifndef KAGOME_SYNCHRONIZER_IMPL_HPP
#define KAGOME_SYNCHRONIZER_IMPL_HPP

#include <memory>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/synchronizer/synchronizer.hpp"
#include "consensus/synchronizer/synchronizer_config.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {

  enum class SynchronizerError { REQUEST_ID_EXIST = 1 };

  class SynchronizerImpl
      : public Synchronizer,
        public std::enable_shared_from_this<SynchronizerImpl> {
    using BlocksResponse = network::BlocksResponse;
    using BlocksRequest = network::BlocksRequest;

   public:
    SynchronizerImpl(
        libp2p::Host &host,
        libp2p::peer::PeerInfo peer_info,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
        SynchronizerConfig config);

    ~SynchronizerImpl() override = default;

    void requestBlocks(
        const BlocksRequest &request,
        std::function<void(outcome::result<BlocksResponse>)> cb) override;

    outcome::result<BlocksResponse> onBlocksRequest(
        const BlocksRequest &request) const override;

   private:
    blockchain::BlockTree::BlockHashVecRes retrieveRequestedHashes(
        const network::BlocksRequest &request,
        const primitives::BlockHash &from_hash) const;

    void fillBlocksResponse(
        const network::BlocksRequest &request,
        network::BlocksResponse &response,
        const std::vector<primitives::BlockHash> &hash_chain) const;

    libp2p::Host &host_;
    libp2p::peer::PeerInfo peer_info_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers_;
    SynchronizerConfig config_;
    std::unordered_set<primitives::BlocksRequestId>
        requested_ids_{};  // requested ids from current peer. TODO: clean after
                           // some period
    common::Logger log_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, SynchronizerError);

#endif  // KAGOME_SYNCHRONIZER_IMPL_HPP
