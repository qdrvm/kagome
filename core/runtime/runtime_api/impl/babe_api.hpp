/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_BABE_API_HPP
#define KAGOME_CORE_RUNTIME_IMPL_BABE_API_HPP

#include "runtime/runtime_api/babe_api.hpp"

#include "common/lru_cache.hpp"

namespace kagome::runtime {

  class Executor;

  class BabeApiImpl final : public BabeApi {
   public:
    explicit BabeApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<primitives::BabeConfiguration> configuration(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;

    LruCache<primitives::BlockHash, primitives::BabeConfiguration> cache_{
        10};
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_BABE_API_HPP
