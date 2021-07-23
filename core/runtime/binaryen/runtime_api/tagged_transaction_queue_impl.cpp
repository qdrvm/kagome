/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"

namespace kagome::runtime::binaryen {
  using primitives::TransactionValidity;

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory)
      : RuntimeApi(runtime_env_factory),
        logger_{log::createLogger("TaggedTransactionQueue", "binarien")} {}

  outcome::result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      primitives::TransactionSource source, const primitives::Extrinsic &ext) {
    logger_->trace("{}", ext.data.toHex());
    return execute<TransactionValidity>(
        "TaggedTransactionQueue_validate_transaction",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        source,
        ext,
        hash);
  }
}  // namespace kagome::runtime::binaryen
