/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_BABE_API_HPP
#define KAGOME_CORE_RUNTIME_IMPL_BABE_API_HPP

#include "runtime/runtime_api/babe_api.hpp"

namespace kagome::runtime {

  class Executor;

  class BabeApiImpl final : public BabeApi {
   public:
    explicit BabeApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<primitives::BabeConfiguration> configuration(
        const primitives::BlockHash &block) override;

    outcome::result<primitives::Epoch> next_epoch(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_BABE_API_HPP
