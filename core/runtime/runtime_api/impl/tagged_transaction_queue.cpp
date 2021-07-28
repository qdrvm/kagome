/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/tagged_transaction_queue.hpp"

#include "blockchain/block_tree.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)},
        logger_{log::createLogger("TaggedTransactionQueue", "runtime")} {
    BOOST_ASSERT(executor_);
  }

  void TaggedTransactionQueueImpl::setBlockTree(
      std::shared_ptr<blockchain::BlockTree> block_tree) {
    block_tree_ = std::move(block_tree);
  }

  outcome::result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      primitives::TransactionSource source, const primitives::Extrinsic &ext) {
    BOOST_ASSERT(block_tree_);
    auto hash = block_tree_->deepestLeaf().hash;
    logger_->trace("Validate transaction called at {}", hash.toHex());
    return executor_->callAt<primitives::TransactionValidity>(
        hash, "TaggedTransactionQueue_validate_transaction", source, ext);
  }

}  // namespace kagome::runtime
