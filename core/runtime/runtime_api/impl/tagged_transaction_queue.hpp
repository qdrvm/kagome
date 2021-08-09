/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_TAGGED_TRANSACTION_QUEUE_HPP
#define KAGOME_RUNTIME_IMPL_TAGGED_TRANSACTION_QUEUE_HPP

#include "runtime/runtime_api/tagged_transaction_queue.hpp"

#include "log/logger.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::runtime {

  class Executor;

  class TaggedTransactionQueueImpl final : public TaggedTransactionQueue {
   public:
    explicit TaggedTransactionQueueImpl(std::shared_ptr<Executor> executor);

    void setBlockTree(std::shared_ptr<blockchain::BlockTree> block_tree);

    outcome::result<primitives::TransactionValidity> validate_transaction(
        primitives::TransactionSource source,
        const primitives::Extrinsic &ext) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_HPP
