/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/sync_protocol_observer_impl.hpp"

#include <boost/assert.hpp>

#include "network/common.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network,
                            SyncProtocolObserverImpl::Error,
                            e) {
  using E = kagome::network::SyncProtocolObserverImpl::Error;
  switch (e) {
    case E::DUPLICATE_REQUEST_ID:
      return "Request with a same id is handling right now";
  }
  return "unknown error";
}

namespace kagome::network {

  SyncProtocolObserverImpl::SyncProtocolObserverImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers)
      : block_tree_{std::move(block_tree)},
        blocks_headers_{std::move(blocks_headers)},
        log_(common::createLogger("SyncProtocolObserver")) {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(blocks_headers_);
  }

  outcome::result<network::BlocksResponse>
  SyncProtocolObserverImpl::onBlocksRequest(
      const BlocksRequest &request) const {
    if (!requested_ids_.emplace(request.id).second) {
      return Error::DUPLICATE_REQUEST_ID;
    }

    BlocksResponse response{request.id};

    // firstly, check if we have both "from" & "to" blocks (if set)
    auto from_hash_res = blocks_headers_->getHashById(request.from);
    if (!from_hash_res) {
      log_->warn("cannot find a requested block with id {}", request.from);
      requested_ids_.erase(request.id);
      return response;
    }

    // secondly, retrieve hashes of blocks the other peer is interested in
    auto chain_hash_res =
        retrieveRequestedHashes(request, from_hash_res.value());
    if (!chain_hash_res) {
      log_->warn("cannot retrieve a chain of blocks: {}",
                 chain_hash_res.error().message());
      requested_ids_.erase(request.id);
      return response;
    }

    // thirdly, fill the resulting response with data, which we were asked for
    fillBlocksResponse(request, response, chain_hash_res.value());
    if (not response.blocks.empty()) {
      log_->debug("Return response: {}", response.blocks[0].hash.toHex());
    }

    requested_ids_.erase(request.id);
    return response;
  }

  blockchain::BlockTree::BlockHashVecRes
  SyncProtocolObserverImpl::retrieveRequestedHashes(
      const BlocksRequest &request,
      const primitives::BlockHash &from_hash) const {
    auto ascending_direction =
        request.direction == network::Direction::ASCENDING;
    blockchain::BlockTree::BlockHashVecRes chain_hash_res{{}};
    if (!request.to) {
      // if there's no "stop" block, get as many as possible
      chain_hash_res = block_tree_->getChainByBlock(
          from_hash, ascending_direction, maxRequestBlocks);
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

  void SyncProtocolObserverImpl::fillBlocksResponse(
      const BlocksRequest &request,
      BlocksResponse &response,
      const std::vector<primitives::BlockHash> &hash_chain) const {
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
}  // namespace kagome::network
