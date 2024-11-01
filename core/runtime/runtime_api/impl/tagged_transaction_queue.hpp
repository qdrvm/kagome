/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/tagged_transaction_queue.hpp"

#include "injector/lazy.hpp"
#include "log/logger.hpp"

namespace kagome::blockchain {
  class BlockTree;
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::runtime {

  class Executor;
  class RuntimeCodeProvider;
  class ModuleFactory;

  class TaggedTransactionQueueImpl final : public TaggedTransactionQueue {
   public:
    TaggedTransactionQueueImpl(std::shared_ptr<Executor> executor,
                               LazySPtr<blockchain::BlockTree> block_tree,
                               std::shared_ptr<RuntimeCodeProvider> cd_prvdr,
                               std::shared_ptr<ModuleFactory> mdl_fctr);

    outcome::result<TransactionValidityAt> validate_transaction(
        primitives::TransactionSource source,
        const primitives::Extrinsic &ext) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<RuntimeCodeProvider> cd_prvdr_;
    std::shared_ptr<ModuleFactory> mdl_fctr_;
    LazySPtr<blockchain::BlockTree> block_tree_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime
