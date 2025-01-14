/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_storage.hpp"

namespace kagome::parachain {
  ParachainStorageImpl::ParachainStorageImpl(
      std::shared_ptr<parachain::AvailabilityStore> av_store)
      : av_store_(std::move(av_store)) {}

  network::ResponsePov ParachainStorageImpl::getPov(
      CandidateHash &&candidate_hash) {
    if (auto res = av_store_->getPov(candidate_hash)) {
      return network::ResponsePov{*res};
    }
    return network::Empty{};
  }

  outcome::result<network::FetchChunkResponse>
  ParachainStorageImpl::OnFetchChunkRequest(
      const network::FetchChunkRequest &request) {
    if (auto chunk =
            av_store_->getChunk(request.candidate, request.chunk_index)) {
      return network::Chunk{
          .data = chunk->chunk,
          .chunk_index = request.chunk_index,
          .proof = chunk->proof,
      };
    }
    return network::Empty{};
  }

  outcome::result<network::FetchChunkResponseObsolete>
  ParachainStorageImpl::OnFetchChunkRequestObsolete(
      const network::FetchChunkRequest &request) {
    if (auto chunk =
            av_store_->getChunk(request.candidate, request.chunk_index)) {
      // This check needed because v1 protocol mustn't have chunk mapping
      // https://github.com/paritytech/polkadot-sdk/blob/d2fd53645654d3b8e12cbf735b67b93078d70113/polkadot/node/core/av-store/src/lib.rs#L1345
      if (chunk->index == request.chunk_index) {
        return network::ChunkObsolete{
            .data = chunk->chunk,
            .proof = chunk->proof,
        };
      }
    }
    return network::Empty{};
  }

}  // namespace kagome::parachain
