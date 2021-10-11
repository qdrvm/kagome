/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_OFFCHAINWORKERAPIIMPL
#define KAGOME_RUNTIME_OFFCHAINWORKERAPIIMPL

#include "runtime/runtime_api/offchain_worker_api.hpp"

namespace kagome::runtime {

  class Executor;

  class OffchainWorkerApiImpl final : public OffchainWorkerApi {
   public:
    explicit OffchainWorkerApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<void> offchain_worker(
        const primitives::BlockHash &block,
        primitives::BlockNumber number) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_OFFCHAINWORKERAPIIMPL
