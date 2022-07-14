/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/sync_protocol_observer_impl.hpp"

#include <boost/assert.hpp>

#include "application/app_configuration.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"

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
        log_(log::createLogger("SyncProtocolObserver", "network")) {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(blocks_headers_);
  }

  outcome::result<network::BlocksResponse>
  SyncProtocolObserverImpl::onBlocksRequest(
      const BlocksRequest &request) const {
    auto request_id = request.fingerprint();
    if (!requested_ids_.emplace(request_id).second) {
      return Error::DUPLICATE_REQUEST_ID;
    }

    BlocksResponse response;

    // firstly, check if we have both "from" & "to" blocks (if set)
    auto from_hash_res = blocks_headers_->getHashById(request.from);
    if (not from_hash_res.has_value()) {
      log_->warn("cannot find a requested block with id {}", request.from);
      requested_ids_.erase(request_id);
      return response;
    }
    const auto &from_hash = from_hash_res.value();

    // secondly, retrieve hashes of blocks the other peer is interested in
    auto chain_hash_res = retrieveRequestedHashes(request, from_hash);
    if (not chain_hash_res.has_value()) {
      log_->warn("cannot retrieve a chain of blocks: {}",
                 chain_hash_res.error().message());
      requested_ids_.erase(request_id);
      return response;
    }
    const auto &chain_hash = chain_hash_res.value();

    // thirdly, fill the resulting response with data, which we were asked for
    fillBlocksResponse(request, response, chain_hash);
    if (response.blocks.empty()) {
      SL_DEBUG(log_, "Return response id={}: no blocks", request_id);
    } else if (response.blocks.size() == 1) {
        SL_DEBUG(
            log_,
            "Return response id={}: {}, count 1",
            request_id, response.blocks.front().hash);
    } else {
      SL_DEBUG(
          log_,
          "Return response id={}: from {} to {}, count {}",
          request_id,
          response.blocks.front().hash,
          response.blocks.back().hash,
          response.blocks.size());
    }

    requested_ids_.erase(request_id);
    return response;
  }

  blockchain::BlockTree::BlockHashVecRes
  SyncProtocolObserverImpl::retrieveRequestedHashes(
      const BlocksRequest &request,
      const primitives::BlockHash &from_hash) const {
    auto direction = request.direction == network::Direction::ASCENDING
                         ? blockchain::BlockTree::GetChainDirection::ASCEND
                         : blockchain::BlockTree::GetChainDirection::DESCEND;
    blockchain::BlockTree::BlockHashVecRes chain_hash_res{{}};

    uint32_t request_count =
        application::AppConfiguration::kAbsolutMaxBlocksInResponse;
    if (request.max.has_value()) {
      request_count = std::clamp(
          request.max.value(),
          application::AppConfiguration::kAbsolutMinBlocksInResponse,
          application::AppConfiguration::kAbsolutMaxBlocksInResponse);
    }

    // Note: request.to is not used in substrate

    switch (direction) {
      case blockchain::BlockTree::GetChainDirection::ASCEND: {
        OUTCOME_TRY(
            chain_hash,
            block_tree_->getBestChainFromBlock(from_hash, request_count));
        return std::move(chain_hash);
      }
      case blockchain::BlockTree::GetChainDirection::DESCEND: {
        OUTCOME_TRY(
            chain_hash,
            block_tree_->getDescendingChainToBlock(from_hash, request_count));
        return std::move(chain_hash);
      }
      default:
        BOOST_UNREACHABLE_RETURN({});
    }
  }

  void SyncProtocolObserverImpl::fillBlocksResponse(
      const BlocksRequest &request,
      BlocksResponse &response,
      const std::vector<primitives::BlockHash> &hash_chain) const {
    auto header_needed =
        request.attributeIsSet(network::BlockAttribute::HEADER);
    auto body_needed = request.attributeIsSet(network::BlockAttribute::BODY);
    auto justification_needed =
        request.attributeIsSet(network::BlockAttribute::JUSTIFICATION);

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
