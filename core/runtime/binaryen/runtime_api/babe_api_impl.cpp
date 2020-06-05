/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/babe_api_impl.hpp"

namespace kagome::runtime::binaryen {

  BabeApiImpl::BabeApiImpl(
      const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(runtime_manager) {}

  outcome::result<primitives::BabeConfiguration> BabeApiImpl::configuration() {
    return execute<primitives::BabeConfiguration>("BabeApi_configuration",
                                                  CallPersistency::EPHEMERAL);
  }

}  // namespace kagome::runtime::binaryen
