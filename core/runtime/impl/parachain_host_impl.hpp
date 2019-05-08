/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP

#include "runtime/parachain_host.hpp"

namespace kagome::runtime {

  class ParachainHostImpl : public ParachainHost {
   public:
    ~ParachainHostImpl() override = default;

    // $ParachainHost_duty_roster
    outcome::result<DutyRoster> dutyRoster() override;

    // $ParachainHost_active_parachains
    outcome::result<std::vector<ParachainId>> activeParachains() override;

    // $ParachainHost_parachain_head
    outcome::result<std::optional<Buffer>> parachainHead(ParachainId id) override;

    // $ParachainHost_parachain_code
    outcome::result<std::optional<Buffer>> parachainCode(ParachainId id) override;

    // reports the set of authorities at a given block
    // receives block_id as argument
    // returns array of authority_id
    // ParachainHost_validators() alias for Core_authorities
    // $Core_authorities ???
    outcome::result<std::vector<ValidatorId>> validators() override{
      return {{}};
    };
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP
