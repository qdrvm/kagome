/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <boost/assert.hpp>
#include "network/types/block_announce.hpp"

namespace kagome::consensus {
  SynchronizerImpl::SynchronizerImpl(
      libp2p::peer::PeerInfo peer_info,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
      SynchronizerConfig config,
      common::Logger log)
      : peer_info_{std::move(peer_info)},
        block_tree_{std::move(block_tree)},
        blocks_headers_{std::move(blocks_headers)},
        config_{config},
        log_{std::move(log)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(blocks_headers_);
    BOOST_ASSERT(log_);
  }

  void SynchronizerImpl::blocksRequest(
      const BlocksRequest &request,
      std::function<void(outcome::result<BlocksResponse>)> cb) const {
    // use SCALE RPC
  }

  void SynchronizerImpl::announce(const primitives::BlockHeader &block_header) {
  }

  outcome::result<network::BlocksResponse> SynchronizerImpl::onBlocksRequest(
      const BlocksRequest &request) const {
    BlocksResponse response{request.id};

    // firstly, check if we have both "from" & "to" blocks (if set)
    auto from_hash_res = blocks_headers_->getHashById(request.from);
    if (!from_hash_res) {
      log_->info("cannot find a requested block with id {}", request.from);
      return response;
    }

    // secondly, retrieve hashes of blocks the other peer is interested in
    auto chain_hash_res =
        retrieveRequestedHashes(request, from_hash_res.value());
    if (!chain_hash_res) {
      log_->error("cannot retrieve a chain of blocks: {}",
                  chain_hash_res.error().message());
      return response;
    }

    // thirdly, fill the resulting response with data, which we were asked for
    fillBlockResponse(request, response, chain_hash_res.value());
    return response;
  }

  blockchain::BlockTree::BlockHashVecRes
  SynchronizerImpl::retrieveRequestedHashes(
      const BlocksRequest &request,
      const primitives::BlockHash &from_hash) const {
    auto ascending_direction =
        request.direction == network::Direction::ASCENDING;
    blockchain::BlockTree::BlockHashVecRes chain_hash_res{{}};
    if (!request.to) {
      // if there's no "stop" block, get as many as possible
      chain_hash_res = block_tree_->getChainByBlock(
          from_hash, ascending_direction, config_.max_request_blocks);
    } else {
      // else, both blocks are specified
      chain_hash_res = block_tree_->getChainByBlocks(
          ascending_direction ? *request.to : from_hash,
          ascending_direction ? from_hash : *request.to);
    }
    return chain_hash_res;
  }

  void SynchronizerImpl::fillBlockResponse(
      const BlocksRequest &request,
      BlocksResponse &response,
      const std::vector<primitives::BlockHash> &hash_chain) const {
    // TODO(akvinikym): understand, where to take receipt and message_queue
    auto header_needed =
        request.attributeIsSet(network::BlockAttributesBits::HEADER);
    auto body_needed =
        request.attributeIsSet(network::BlockAttributesBits::BODY);
    auto justification_needed =
        request.attributeIsSet(network::BlockAttributesBits::JUSTIFICATION);
    for (const auto &hash : hash_chain) {
      auto &new_block = response.blocks.emplace_back(network::BlockData{hash});

      if (header_needed) {
        auto header_res = blocks_headers_->getBlockHeader(hash);
        if (header_res) {
          new_block.header = std::move(header_res.value());
        }
      }
      if (body_needed) {
        auto body_res = block_tree_->getBlockBody(hash);
        if (body_res) {
          new_block.body = std::move(body_res.value());
        }
      }
      if (justification_needed) {
        auto justification_res = block_tree_->getBlockJustification(hash);
        if (justification_res) {
          new_block.justification = std::move(justification_res.value());
        }
      }
    }
  }
}  // namespace kagome::consensus
