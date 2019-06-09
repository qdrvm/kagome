/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/offchain_worker_impl.hpp"

#include "runtime/impl/runtime_api.hpp"
#include "runtime/impl/runtime_external_interface.hpp"

namespace kagome::runtime {
  OffchainWorkerImpl::OffchainWorkerImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension) {
    runtime_ = std::make_unique<RuntimeApi>(std::move(state_code),
                                            std::move(extension));
  }

  OffchainWorkerImpl::~OffchainWorkerImpl() {}

  outcome::result<void> OffchainWorkerImpl::offchain_worker(BlockNumber bn) {
    return runtime_->execute("OffchainWorkerApi_offchain_worker", bn);
  }
}  // namespace kagome::runtime
