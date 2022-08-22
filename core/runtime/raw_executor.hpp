/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {

  class RawExecutor {
   public:
    using Buffer = common::Buffer;
    using BlockHash = primitives::BlockHash;
    virtual ~RawExecutor() = default;

    virtual outcome::result<Buffer> callAtRaw(const BlockHash &block_hash,
                                              std::string_view name,
                                              const Buffer &encoded_args) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP
