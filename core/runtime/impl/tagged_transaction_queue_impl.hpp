/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP

#include <outcome/outcome.hpp>
#include "extensions/extension.hpp"
#include "runtime/tagged_transaction_queue.hpp"

namespace kagome::runtime {
  class RuntimeApi;

  class TaggedTransactionQueueImpl : public TaggedTransactionQueue {
   public:
    TaggedTransactionQueueImpl(
        common::Buffer state_code,
        std::shared_ptr<extensions::Extension> extension);

    ~TaggedTransactionQueueImpl() override;

    outcome::result<primitives::TransactionValidity> validate_transaction(
        const primitives::Extrinsic &ext) override;

   private:
    std::unique_ptr<RuntimeApi> runtime_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP
