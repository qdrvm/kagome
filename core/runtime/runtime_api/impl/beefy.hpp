/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/beefy.hpp"

namespace kagome::runtime {
  class Executor;

  class BeefyApiImpl : public BeefyApi {
   public:
    BeefyApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<std::optional<primitives::BlockNumber>> genesis(
        const primitives::BlockHash &block) override;
    outcome::result<std::optional<consensus::beefy::ValidatorSet>> validatorSet(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;
  };
}  // namespace kagome::runtime
