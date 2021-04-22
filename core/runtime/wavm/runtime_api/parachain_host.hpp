/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_WAVM_PARACHAIN_HOST_HPP

#include "primitives/block_id.hpp"
#include "runtime/parachain_host.hpp"

namespace kagome::runtime::wavm {

  class WavmParachainHost final: public ParachainHost {
   public:
    WavmParachainHost(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<DutyRoster> duty_roster() override {
      return executor_->call<DutyRoster>(
          "ParachainHost_duty_roster");
    }

    outcome::result<std::vector<ParachainId>> active_parachains() override {
      return executor_->call<std::vector<ParachainId>>(
          "ParachainHost_active_parachains");
    }

    outcome::result<boost::optional<Buffer>> parachain_head(
        ParachainId id) override {
      return executor_->call<boost::optional<Buffer>>(
          "ParachainHost_parachain_head", id);
    }

    outcome::result<boost::optional<kagome::common::Buffer>>
    parachain_code(ParachainId id) override {
      return executor_->call<boost::optional<kagome::common::Buffer>>(
          "ParachainHost_parachain_code", id);
    }

    outcome::result<std::vector<ValidatorId>> validators() override {
      return executor_->call<std::vector<ValidatorId>>(
          "ParachainHost_validators");
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
