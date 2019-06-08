/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP

#include "runtime/parachain_host.hpp"

#include <outcome/outcome.hpp>
#include "extensions/extension.hpp"
#include "runtime/tagged_transaction_queue.hpp"

namespace kagome::runtime {
  class RuntimeApi;

  class ParachainHostImpl : public ParachainHost {
   public:
    /**
     * @brief constructor
     * @param state_code error or result code
     * @param extension extension instance
     * @param codec scale codec instance
     */
    ParachainHostImpl(common::Buffer state_code,
                      std::shared_ptr<extensions::Extension> extension);

    ~ParachainHostImpl() override;

    outcome::result<DutyRoster> duty_roster() override;

    outcome::result<std::vector<ParachainId>> active_parachains() override;

    outcome::result<std::optional<Buffer>> parachain_head(
        ParachainId id) override;

    outcome::result<std::optional<Buffer>> parachainCode(
        ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators() override;

   private:
    std::unique_ptr<RuntimeApi> runtime_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP
