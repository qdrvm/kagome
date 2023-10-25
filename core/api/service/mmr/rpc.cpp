/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/mmr/rpc.hpp"

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/jrpc_fn.hpp"
#include "blockchain/block_tree.hpp"
#include "offchain/offchain_worker_factory.hpp"
#include "offchain/offchain_worker_pool.hpp"
#include "runtime/runtime_api/mmr.hpp"

namespace kagome::api {
  template <typename T>
  inline T unwrap(primitives::MmrResult<T> &&r) {
    using E = primitives::MmrError;
    if (auto e = boost::get<E>(&r)) {
      int32_t code = 8010;
      switch (*e) {
        case E::LeafNotFound:
          code += 1;
          break;
        case E::GenerateProof:
          code += 2;
          break;
        case E::Verify:
          code += 3;
          break;
        case E::InvalidNumericOp:
          code += 4;
          break;
        case E::InvalidBestKnownBlock:
          code += 5;
          break;
        default:
          break;
      }
      throw jsonrpc::Fault{
          fmt::format("MmrError({})", static_cast<uint8_t>(*e)),
          code,
      };
    }
    return boost::get<T>(std::move(r));
  }

  MmrRpc::MmrRpc(
      std::shared_ptr<JRpcServer> server,
      LazySPtr<runtime::MmrApi> mmr_api,
      LazySPtr<blockchain::BlockTree> block_tree,
      LazySPtr<runtime::Executor> executor,
      LazySPtr<offchain::OffchainWorkerFactory> offchain_worker_factory,
      LazySPtr<offchain::OffchainWorkerPool> offchain_worker_pool)
      : server_{std::move(server)},
        mmr_api_{std::move(mmr_api)},
        block_tree_{std::move(block_tree)},
        executor_{std::move(executor)},
        offchain_worker_factory_{std::move(offchain_worker_factory)},
        offchain_worker_pool_{std::move(offchain_worker_pool)} {}

  auto MmrRpc::withOffchain(const primitives::BlockHash &at) {
    // TODO(turuslan): simplify offchain
    auto remove =
        gsl::finally([&] { offchain_worker_pool_.get()->removeWorker(); });
    outcome::result<decltype(remove)> result{std::move(remove)};
    if (auto r = block_tree_.get()->getBlockHeader(at)) {
      offchain_worker_pool_.get()->addWorker(
          offchain_worker_factory_.get()->make(executor_.get(), r.value()));
      return result;
    } else {
      return decltype(result){r.error()};
    }
  }

  void MmrRpc::registerHandlers() {
    server_->registerHandler(
        "mmr_root",
        jrpcFn(this,
               [](std::shared_ptr<MmrRpc> self,
                  std::optional<primitives::BlockHash> at)
                   -> outcome::result<common::Hash256> {
                 if (not at) {
                   at = self->block_tree_.get()->bestBlock().hash;
                 }
                 OUTCOME_TRY(r, self->mmr_api_.get()->mmrRoot(*at));
                 return unwrap(std::move(r));
               }));

    server_->registerHandler(
        "mmr_generateProof",
        jrpcFn(
            this,
            [](std::shared_ptr<MmrRpc> self,
               std::vector<primitives::BlockNumber> block_numbers,
               std::optional<primitives::BlockNumber> best_known_block_number,
               std::optional<primitives::BlockHash> at)
                -> outcome::result<primitives::MmrLeavesProof> {
              if (not at) {
                at = self->block_tree_.get()->bestBlock().hash;
              }
              auto offchain = self->withOffchain(*at);
              OUTCOME_TRY(r,
                          self->mmr_api_.get()->generateProof(
                              *at, block_numbers, best_known_block_number));
              auto [leaves, proof] = unwrap(std::move(r));
              return primitives::MmrLeavesProof{
                  *at,
                  common::Buffer{scale::encode(leaves).value()},
                  common::Buffer{scale::encode(proof).value()},
              };
            }));

    server_->registerHandler(
        "mmr_verifyProof",
        jrpcFn(
            this,
            [](std::shared_ptr<MmrRpc> self,
               primitives::MmrLeavesProof proof_raw) -> outcome::result<bool> {
              auto &at = proof_raw.block_hash;
              OUTCOME_TRY(leaves,
                          scale::decode<primitives::MmrLeaves>(
                              proof_raw.leaves.view()));
              OUTCOME_TRY(
                  proof,
                  scale::decode<primitives::MmrProof>(proof_raw.proof.view()));
              auto offchain = self->withOffchain(at);
              OUTCOME_TRY(r,
                          self->mmr_api_.get()->verifyProof(at, leaves, proof));
              unwrap(std::move(r));
              return true;
            }));

    server_->registerHandler(
        "mmr_verifyProofStateless",
        jrpcFn(
            this,
            [](std::shared_ptr<MmrRpc> self,
               primitives::BlockHash mmr_root,
               primitives::MmrLeavesProof proof_raw) -> outcome::result<bool> {
              auto &at = proof_raw.block_hash;
              OUTCOME_TRY(leaves,
                          scale::decode<primitives::MmrLeaves>(
                              proof_raw.leaves.view()));
              OUTCOME_TRY(
                  proof,
                  scale::decode<primitives::MmrProof>(proof_raw.proof.view()));
              OUTCOME_TRY(r,
                          self->mmr_api_.get()->verifyProofStateless(
                              at, mmr_root, leaves, proof));
              unwrap(std::move(r));
              return true;
            }));
  }
}  // namespace kagome::api
