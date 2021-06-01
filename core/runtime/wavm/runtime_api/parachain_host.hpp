/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_WAVM_PARACHAIN_HOST_HPP

#include "primitives/block_id.hpp"
#include "runtime/parachain_host.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmParachainHost final: public ParachainHost {
   public:
    WavmParachainHost(std::shared_ptr<Executor> executor);

    outcome::result<DutyRoster> duty_roster() override;

    outcome::result<std::vector<ParachainId>> active_parachains() override;

    outcome::result<boost::optional<Buffer>> parachain_head(
        ParachainId id) override;

    outcome::result<boost::optional<kagome::common::Buffer>>
    parachain_code(ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators() override;

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
