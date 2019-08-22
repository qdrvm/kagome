/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "extensions/extension.hpp"
#include "runtime/impl/runtime_api.hpp"
#include "runtime/tagged_transaction_queue.hpp"

namespace kagome::runtime {
  class TaggedTransactionQueueImpl : public RuntimeApi,
                                     public TaggedTransactionQueue {
   public:
    TaggedTransactionQueueImpl(
        common::Buffer state_code,
        std::shared_ptr<extensions::Extension> extension,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_backend);

    ~TaggedTransactionQueueImpl() override = default;

    outcome::result<primitives::TransactionValidity> validate_transaction(
        const primitives::Extrinsic &ext) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_backend_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP
