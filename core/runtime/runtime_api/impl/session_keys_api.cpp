/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/session_keys_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  SessionKeysApiImpl::SessionKeysApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<common::Buffer> SessionKeysApiImpl::generate_session_keys(
      const primitives::BlockHash &block_hash,
      std::optional<common::Buffer> seed) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<common::Buffer, std::optional<common::Buffer>>(
        ctx, "SessionKeys_generate_session_keys", seed);
  }

  outcome::result<SessionKeysApi::DecodeSessionKeysResult>
  SessionKeysApiImpl::decode_session_keys(
      const primitives::BlockHash &block_hash,
      common::BufferView encoded) const {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<SessionKeysApi::DecodeSessionKeysResult>(
        ctx, "SessionKeys_decode_session_keys", encoded);
  }

}  // namespace kagome::runtime
