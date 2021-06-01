/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/babe_api.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmBabeApi::WavmBabeApi(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::BabeConfiguration> WavmBabeApi::configuration() {
    return executor_->callAtLatest<primitives::BabeConfiguration>(
        "BabeApi_configuration");
  }

}  // namespace kagome::runtime::wavm
