/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_BABE_API_HPP
#define KAGOME_CORE_RUNTIME_WAVM_BABE_API_HPP

#include "runtime/babe_api.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmBabeApi final: public BabeApi {
   public:
    WavmBabeApi(std::shared_ptr<Executor> executor);

    outcome::result<primitives::BabeConfiguration> configuration();

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_BABE_API_HPP
