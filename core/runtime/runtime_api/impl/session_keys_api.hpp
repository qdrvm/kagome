/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_SESSIONKEYSAPIIMPL
#define KAGOME_RUNTIME_SESSIONKEYSAPIIMPL

#include "runtime/runtime_api/session_keys_api.hpp"

namespace kagome::runtime {

  class Executor;

  class SessionKeysApiImpl final : public SessionKeysApi {
   public:
    explicit SessionKeysApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<common::Buffer> generate_session_keys(
        const primitives::BlockHash &block_hash,
        std::optional<common::Buffer> seed) override;

    outcome::result<std::vector<std::pair<crypto::KeyTypeId, common::Buffer>>>
    decode_session_keys(const primitives::BlockHash &block_hash,
                        common::BufferView encoded) const override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_SESSIONKEYSAPIIMPL
