/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP

#include <cstdint>
#include <vector>

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "common/buffer.hpp"

namespace kagome::primitives::parachain {
  class ValidatorId {
    int a;
  };

  using ParachainId = uint32_t;

  struct Relay {};  // empty

  struct Parachain {
    ParachainId id_;
  };

  using Chain = boost::variant<Relay, Parachain>;

  using DutyRoster = std::vector<Chain>;

}  // namespace kagome::primitives::parachain

namespace kagome::runtime {

  class ParachainHost {
   protected:
    using Buffer = common::Buffer;
    using ValidatorId = primitives::parachain::ValidatorId;
    using DutyRoster = primitives::parachain::DutyRoster;
    using ParachainId = primitives::parachain::ParachainId;

   public:
    virtual ~ParachainHost() = default;

    // $ParachainHost_duty_roster
    virtual outcome::result<DutyRoster> dutyRoster() = 0;

    // $ParachainHost_active_parachains
    virtual outcome::result<std::vector<ParachainId>> activeParachains() = 0;

    // $ParachainHost_parachain_head
    virtual outcome::result<std::optional<Buffer>> parachainHead(
        ParachainId id) = 0;

    // $ParachainHost_parachain_code
    virtual outcome::result<std::optional<kagome::common::Buffer>>
    parachainCode(ParachainId id) = 0;

    // reports the set of authorities at a given block
    // receives block_id as argument
    // returns array of authority_id
    // ParachainHost_validators() alias for Core_authorities
    // $Core_authorities ???
    virtual outcome::result<std::vector<ValidatorId>> validators() = 0;
  };

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
