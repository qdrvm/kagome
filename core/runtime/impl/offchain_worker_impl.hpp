/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_IMPL_HPP

#include "runtime/offchain_worker.hpp"

#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "extensions/extension.hpp"
#include "runtime/impl/wasm_executor.hpp"

namespace kagome::runtime {

  class OffchainWorkerImpl : public OffchainWorker {
   protected:
   public:
    ~OffchainWorkerImpl() override = default;

    OffchainWorkerImpl(common::Buffer state_code,
                       std::shared_ptr<extensions::Extension> extension);

    outcome::result<void> offchain_worker(BlockNumber bn) override;

   private:
    common::Buffer state_code_;
    std::shared_ptr<WasmMemory> memory_;
    WasmExecutor executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_OFFCHAIN_WORKER_IMPL_HPP
