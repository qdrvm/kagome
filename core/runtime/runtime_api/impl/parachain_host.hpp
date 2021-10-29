/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_HPP

#include "primitives/block_id.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::runtime {

  class Executor;

  class ParachainHostImpl final : public ParachainHost {
   public:
    explicit ParachainHostImpl(std::shared_ptr<Executor> executor);

    outcome::result<DutyRoster> duty_roster(
        const primitives::BlockHash &block) override;

    outcome::result<std::vector<ParachainId>> active_parachains(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<Buffer>> parachain_head(
        const primitives::BlockHash &block, ParachainId id) override;

    outcome::result<std::optional<kagome::common::Buffer>> parachain_code(
        const primitives::BlockHash &block, ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
