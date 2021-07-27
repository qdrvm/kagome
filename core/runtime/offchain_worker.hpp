/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_OFFCHAIN_WORKER_HPP
#define KAGOME_CORE_RUNTIME_OFFCHAIN_WORKER_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {

  class OffchainWorker {
   protected:
    using BlockNumber = kagome::primitives::BlockNumber;

   public:
    virtual ~OffchainWorker() = default;

    /**
     * @brief calls offchain_worker method of OffchainWorker runtime api
     * @param bn block number
     * @return success or error
     */
    virtual outcome::result<void> offchain_worker(BlockNumber bn) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_OFFCHAIN_WORKER_HPP
