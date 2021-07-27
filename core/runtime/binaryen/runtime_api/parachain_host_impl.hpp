/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_PARACHAIN_HOST_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_PARACHAIN_HOST_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/parachain_host.hpp"

namespace kagome::runtime::binaryen {

  class ParachainHostImpl : public RuntimeApi, public ParachainHost {
   public:
    /**
     * @brief constructor
     * @param state_code error or result code
     * @param extension extension instance
     * @param codec scale codec instance
     */
    ParachainHostImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory);

    ~ParachainHostImpl() override = default;

    outcome::result<DutyRoster> duty_roster() override;

    outcome::result<std::vector<ParachainId>> active_parachains() override;

    outcome::result<boost::optional<Buffer>> parachain_head(
        ParachainId id) override;

    outcome::result<boost::optional<Buffer>> parachain_code(
        ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators() override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_PARACHAIN_HOST_IMPL_HPP
