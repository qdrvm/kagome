/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/runtime_dispatch_info.hpp"

namespace kagome::runtime {

  class TransactionPaymentApi {
   public:
    virtual ~TransactionPaymentApi() = default;

    virtual outcome::result<
        primitives::RuntimeDispatchInfo<primitives::Weight>>
    query_info(const primitives::BlockHash &block,
               const primitives::Extrinsic &ext,
               uint32_t len) = 0;
  };

}  // namespace kagome::runtime
