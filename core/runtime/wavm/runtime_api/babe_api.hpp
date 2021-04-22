/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_BABE_API_HPP
#define KAGOME_CORE_RUNTIME_WAVM_BABE_API_HPP

#include "runtime/babe_api.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmBabeApi final: public BabeApi {
   public:
    WavmBabeApi(std::shared_ptr<Executor> executor)
        : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<primitives::BabeConfiguration> configuration() {
      return executor_->call<primitives::BabeConfiguration>(
          "BabeApi_configuration");
    }

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_BABE_API_HPP
