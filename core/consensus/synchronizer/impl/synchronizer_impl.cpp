/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <boost/assert.hpp>
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/rpc.hpp"
#include "network/types/block_announce.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, SynchronizerError, e) {
  using E = kagome::consensus::SynchronizerError;
  switch (e) {
    case E::REQUEST_ID_EXIST:
      return "Either peer requests himself, or request was already processed";
  }
  return "unknown error";
}

namespace kagome::consensus {
  SynchronizerImpl::SynchronizerImpl(
      libp2p::Host &host,
      libp2p::peer::PeerInfo peer_info,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
      SynchronizerConfig config)
      : host_{host},
        peer_info_{std::move(peer_info)},
        block_tree_{std::move(block_tree)},
        blocks_headers_{std::move(blocks_headers)},
        config_{config},
        log_(common::createLogger("Synchronizer")) {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(blocks_headers_);
  }

  void SynchronizerImpl::requestBlocks(
      const BlocksRequest &request,
      std::function<void(outcome::result<BlocksResponse>)> cb) {
    visit_in_place(request.from,
                   [this](primitives::BlockNumber from) {
                     log_->debug("Requesting blocks: from {}", from);
                   },
                   [this, &request](const primitives::BlockHash &from) {
                     if (not request.to) {
                       log_->debug("Requesting blocks: from {}", from.toHex());
                     } else {
                       log_->debug("Requesting blocks: from {}, to {}",
                                   from.toHex(),
                                   request.to->toHex());
                     }
                   });
    requested_ids_.insert(request.id);
    network::RPC<network::ScaleMessageReadWriter>::write<BlocksRequest,
                                                         BlocksResponse>(
        host_, peer_info_, network::kSyncProtocol, request, std::move(cb));
  }

  outcome::result<network::BlocksResponse> SynchronizerImpl::onBlocksRequest(
      const BlocksRequest &request) const {
    if (requested_ids_.find(request.id) != requested_ids_.end()) {
      return SynchronizerError::REQUEST_ID_EXIST;
    }
    BlocksResponse response{request.id};

    // firstly, check if we have both "from" & "to" blocks (if set)
    auto from_hash_res = blocks_headers_->getHashById(request.from);
    if (!from_hash_res) {
      log_->warn("cannot find a requested block with id {}", request.from);
      return response;
    }

    // secondly, retrieve hashes of blocks the other peer is interested in
    auto chain_hash_res =
        retrieveRequestedHashes(request, from_hash_res.value());
    if (!chain_hash_res) {
      log_->warn("cannot retrieve a chain of blocks: {}",
                  chain_hash_res.error().message());
      return response;
    }

    // thirdly, fill the resulting response with data, which we were asked for
    fillBlocksResponse(request, response, chain_hash_res.value());
    if (not response.blocks.empty()) {
      log_->debug("Return response: {}", response.blocks[0].hash.toHex());
    }
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
      OUTCOME_TRY(chain_hash,
                  block_tree_->getChainByBlocks(from_hash, *request.to));
      if (ascending_direction) {
        std::reverse(chain_hash.begin(), chain_hash.end());
      }
      chain_hash_res = chain_hash;
    }
    return chain_hash_res;
  }

  void SynchronizerImpl::fillBlocksResponse(
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
      auto &new_block =
          response.blocks.emplace_back(primitives::BlockData{hash});

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
