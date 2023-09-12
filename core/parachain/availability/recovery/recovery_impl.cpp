/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/recovery/recovery_impl.hpp"

#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"

namespace kagome::parachain {
  constexpr size_t kParallelRequests = 50;

  RecoveryImpl::RecoveryImpl(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<AvailabilityStore> av_store,
      std::shared_ptr<authority_discovery::Query> query_audi,
      std::shared_ptr<network::Router> router)
      : hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        parachain_api_{std::move(parachain_api)},
        av_store_{std::move(av_store)},
        query_audi_{std::move(query_audi)},
        router_{std::move(router)} {}

  void RecoveryImpl::remove(const CandidateHash &candidate) {
    std::unique_lock lock{mutex_};
    active_.erase(candidate);
    cached_.erase(candidate);
  }

  void RecoveryImpl::recover(const HashedCandidateReceipt &hashed_receipt,
                             SessionIndex session_index,
                             std::optional<GroupIndex> backing_group,
                             Cb cb) {
    std::unique_lock lock{mutex_};
    const auto &receipt = hashed_receipt.get();
    const auto &candidate_hash = hashed_receipt.getHash();
    if (auto it = cached_.find(candidate_hash); it != cached_.end()) {
      auto r = it->second;
      lock.unlock();
      cb(std::move(r));
      return;
    }
    if (auto it = active_.find(candidate_hash); it != active_.end()) {
      it->second.cb.emplace_back(std::move(cb));
      return;
    }
    if (auto data = av_store_->getPovAndData(candidate_hash)) {
      cached_.emplace(candidate_hash, *data);
      lock.unlock();
      cb(std::move(*data));
      return;
    }
    auto block = block_tree_->bestLeaf();
    auto _session = parachain_api_->session_info(block.hash, session_index);
    if (not _session) {
      lock.unlock();
      cb(_session.error());
      return;
    }
    auto &session = _session.value();
    auto _min = minChunks(session->validators.size());
    if (not _min) {
      lock.unlock();
      cb(_min.error());
      return;
    }
    Active active;
    active.erasure_encoding_root = receipt.descriptor.erasure_encoding_root;
    active.chunks_required = _min.value();
    active.cb.emplace_back(std::move(cb));
    active.validators = session->discovery_keys;
    if (backing_group) {
      active.order = session->validator_groups.at(*backing_group);
      std::shuffle(active.order.begin(), active.order.end(), random_);
    }
    active_.emplace(candidate_hash, std::move(active));
    lock.unlock();
    back(candidate_hash);
  }

  void RecoveryImpl::back(const CandidateHash &candidate_hash) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    while (not active.order.empty()) {
      auto peer = query_audi_->get(active.validators[active.order.back()]);
      active.order.pop_back();
      if (peer) {
        router_->getFetchAvailableDataProtocol()->doRequest(
            peer->id,
            candidate_hash,
            [=, weak{weak_from_this()}](
                outcome::result<network::FetchAvailableDataResponse> r) {
              auto self = weak.lock();
              if (not self) {
                return;
              }
              self->back(candidate_hash, std::move(r));
            });
        return;
      }
    }
    active.chunks = av_store_->getChunks(candidate_hash);
    for (size_t i = 0; i < active.validators.size(); ++i) {
      if (std::find_if(active.chunks.begin(),
                       active.chunks.end(),
                       [&](network::ErasureChunk &c) { return c.index == i; })
          != active.chunks.end()) {
        continue;
      }
      active.order.emplace_back(i);
    }
    std::shuffle(active.order.begin(), active.order.end(), random_);
    lock.unlock();
    chunk(candidate_hash);
  }

  void RecoveryImpl::back(
      const CandidateHash &candidate_hash,
      outcome::result<network::FetchAvailableDataResponse> _backed) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    if (_backed) {
      if (auto data = boost::get<AvailableData>(&_backed.value())) {
        if (check(active, *data)) {
          return done(lock, it, std::move(*data));
        }
      }
    }
    lock.unlock();
    back(candidate_hash);
  }

  void RecoveryImpl::chunk(const CandidateHash &candidate_hash) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    if (active.chunks.size() >= active.chunks_required) {
      auto _data = fromChunks(active.validators.size(), active.chunks);
      if (_data) {
        if (auto r = check(active, _data.value()); not r) {
          _data = r.error();
        }
      }
      return done(lock, it, _data);
    }
    if (active.chunks.size() + active.chunks_active + active.order.size()
        < active.chunks_required) {
      return done(
          lock,
          it,
          ErasureCodingError{toCodeError(ec_cpp::Error::kNeedMoreShards)});
    }
    auto max = std::min(kParallelRequests,
                        active.chunks_required - active.chunks.size());
    while (not active.order.empty() and active.chunks_active < max) {
      auto index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.validators[index]);
      if (peer) {
        ++active.chunks_active;
        router_->getFetchChunkProtocol()->doRequest(
            peer->id,
            {candidate_hash, index},
            [=, weak{weak_from_this()}](
                outcome::result<network::FetchChunkResponse> r) {
              auto self = weak.lock();
              if (not self) {
                return;
              }
              self->chunk(candidate_hash, index, std::move(r));
            });
        return;
      }
    }
    if (active.chunks_active == 0) {
      done(lock, it, std::nullopt);
    }
  }

  void RecoveryImpl::chunk(
      const CandidateHash &candidate_hash,
      ValidatorIndex index,
      outcome::result<network::FetchChunkResponse> _chunk) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    --active.chunks_active;
    if (_chunk) {
      if (auto chunk2 = boost::get<network::Chunk>(&_chunk.value())) {
        network::ErasureChunk chunk{
            std::move(chunk2->data), index, std::move(chunk2->proof)};
        if (checkTrieProof(chunk, active.erasure_encoding_root)) {
          active.chunks.emplace_back(std::move(chunk));
        }
      }
    }
    lock.unlock();
    chunk(candidate_hash);
  }

  outcome::result<void> RecoveryImpl::check(const Active &active,
                                            const AvailableData &data) {
    OUTCOME_TRY(chunks, toChunks(active.validators.size(), data));
    auto root = makeTrieProof(chunks);
    if (root != active.erasure_encoding_root) {
      return ErasureCodingRootError::MISMATCH;
    }
    return outcome::success();
  }

  void RecoveryImpl::done(
      Lock &lock,
      ActiveMap::iterator it,
      const std::optional<outcome::result<AvailableData>> &result) {
    if (result) {
      cached_.emplace(it->first, *result);
    }
    auto node = active_.extract(it);
    lock.unlock();
    for (auto &cb : node.mapped().cb) {
      cb(result);
    }
  }
}  // namespace kagome::parachain
