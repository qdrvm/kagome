/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

namespace kagome::runtime {
  using primitives::TransactionValidity;

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_backend)
      : RuntimeApi(std::move(state_code), std::move(extension)),
        block_tree_(std::move(block_tree)),
        header_backend_(std::move(header_backend)) {}

  outcome::result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      const primitives::Extrinsic &ext) {
    auto &hash = block_tree_->deepestLeaf();
    OUTCOME_TRY(number, header_backend_->getNumberByHash(hash));

    return execute<TransactionValidity>(
        "TaggedTransactionQueue_validate_transaction", number, ext);
  }
}  // namespace kagome::runtime
