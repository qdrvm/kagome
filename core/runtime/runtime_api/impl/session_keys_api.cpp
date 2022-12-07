/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/session_keys_api.hpp"

#include "runtime/common/executor.hpp"

namespace kagome::runtime {

  SessionKeysApiImpl::SessionKeysApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<common::Buffer> SessionKeysApiImpl::generate_session_keys(
      const primitives::BlockHash &block_hash,
      std::optional<common::Buffer> seed) {
    return executor_->callAt<common::Buffer, std::optional<common::Buffer>>(
        block_hash, "SessionKeys_generate_session_keys", std::move(seed));
  }

  outcome::result<std::vector<std::pair<crypto::KeyTypeId, common::Buffer>>>
  SessionKeysApiImpl::decode_session_keys(
      const primitives::BlockHash &block_hash,
      common::BufferView encoded) const {
    return executor_
        ->callAt<std::vector<std::pair<crypto::KeyTypeId, common::Buffer>>>(
            block_hash, "SessionKeys_decode_session_keys", encoded);
  }

}  // namespace kagome::runtime
