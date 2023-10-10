/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/session_keys_api.hpp"

namespace kagome::runtime {

  class Executor;

  class SessionKeysApiImpl final : public SessionKeysApi {
   public:
    explicit SessionKeysApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<common::Buffer> generate_session_keys(
        const primitives::BlockHash &block_hash,
        std::optional<common::Buffer> seed) override;

    outcome::result<std::vector<std::pair<crypto::KeyType, common::Buffer>>>
    decode_session_keys(const primitives::BlockHash &block_hash,
                        common::BufferView encoded) const override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime
