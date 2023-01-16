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
    using OnDbRead = std::function<void(common::BufferView)>;

    virtual ~RawExecutor() = default;

    /**
     * Call a runtime method \param name at state on block \param block_hash in
     * an ephemeral environment, e. g. the storage changes, made by this call,
     * will NOT persist in the node's Trie storage The call will be done with
     * the runtime code from \param block_info state Arguments for the call are
     * expected to be scale-encoded into single buffer \param encoded_args
     * beforehand \returns scale-encoded result
     */
    virtual outcome::result<Buffer> callAtRaw(const BlockHash &block_hash,
                                              std::string_view name,
                                              const Buffer &encoded_args,
                                              OnDbRead on_db_read) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP
