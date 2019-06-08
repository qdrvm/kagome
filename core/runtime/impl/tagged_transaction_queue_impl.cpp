/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  using primitives::TransactionValidity;

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension) {
    runtime_ = std::make_unique<RuntimeApi>(std::move(state_code),
                                            std::move(extension));
  }

  TaggedTransactionQueueImpl::~TaggedTransactionQueueImpl() {}

  outcome::result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      const primitives::Extrinsic &ext) {
    return runtime_->execute<TransactionValidity>(
        "TaggedTransactionQueue_validate_transaction", ext);
  }

}  // namespace kagome::runtime
