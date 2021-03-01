/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/babe_api_impl.hpp"

namespace kagome::runtime::binaryen {

  BabeApiImpl::BabeApiImpl(
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory)
      : RuntimeApi(runtime_env_factory) {}

  outcome::result<primitives::BabeConfiguration> BabeApiImpl::configuration() {
    return execute<primitives::BabeConfiguration>(
        "BabeApi_configuration",
        CallConfig{.persistency = CallPersistency::EPHEMERAL});
  }

}  // namespace kagome::runtime::binaryen
