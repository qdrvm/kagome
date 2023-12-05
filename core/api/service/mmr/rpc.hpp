/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"
#include "injector/lazy.hpp"
#include "primitives/common.hpp"

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::offchain {
  class OffchainWorkerFactory;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

namespace kagome::runtime {
  class MmrApi;
  class Executor;
}  // namespace kagome::runtime

namespace kagome::api {
  class MmrRpc : public JRpcProcessor,
                 public std::enable_shared_from_this<MmrRpc> {
   public:
    MmrRpc(std::shared_ptr<JRpcServer> server,
           LazySPtr<runtime::MmrApi> mmr_api,
           LazySPtr<blockchain::BlockTree> block_tree,
           LazySPtr<runtime::Executor> executor,
           LazySPtr<offchain::OffchainWorkerFactory> offchain_worker_factory,
           LazySPtr<offchain::OffchainWorkerPool> offchain_worker_pool);

    void registerHandlers() override;

   private:
    auto withOffchain(const primitives::BlockHash &at);

    std::shared_ptr<JRpcServer> server_;
    LazySPtr<runtime::MmrApi> mmr_api_;
    LazySPtr<blockchain::BlockTree> block_tree_;
    LazySPtr<runtime::Executor> executor_;
    LazySPtr<offchain::OffchainWorkerFactory> offchain_worker_factory_;
    LazySPtr<offchain::OffchainWorkerPool> offchain_worker_pool_;
  };
}  // namespace kagome::api
