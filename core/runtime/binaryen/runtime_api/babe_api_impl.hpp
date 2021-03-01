/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_BABE_API_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_BABE_API_IMPL_HPP

#include "runtime/babe_api.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"

namespace kagome::runtime::binaryen {

  class BabeApiImpl : public RuntimeApi, public BabeApi {
   public:
    ~BabeApiImpl() override = default;

    explicit BabeApiImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory);

    outcome::result<primitives::BabeConfiguration> configuration() override;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_BABE_API_IMPL_HPP
