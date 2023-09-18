/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/grandpa_api.hpp"

namespace kagome::runtime {

  class Executor;

  class GrandpaApiImpl final : public GrandpaApi {
   public:
    GrandpaApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<AuthorityList> authorities(
        const primitives::BlockHash &block_hash) override;

    outcome::result<primitives::AuthoritySetId> current_set_id(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime
