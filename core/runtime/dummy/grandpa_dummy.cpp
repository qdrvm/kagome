/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/dummy/grandpa_dummy.hpp"

namespace kagome::runtime::dummy {

  GrandpaDummy::GrandpaDummy(std::vector<WeightedAuthority> authorities)
      : authorities_{std::move(authorities)} {}

  outcome::result<boost::optional<Grandpa::ScheduledChange>>
  GrandpaDummy::pending_change(const Digest &digest) {
    BOOST_ASSERT_MSG(false, "Grandpa::pending_change is not implemented");
  }

  outcome::result<boost::optional<Grandpa::ForcedChange>>
  GrandpaDummy::forced_change(const Digest &digest) {
    BOOST_ASSERT_MSG(false, "Grandpa::forced_change is not implemented");
  }

  outcome::result<std::vector<Grandpa::WeightedAuthority>>
  GrandpaDummy::authorities(const primitives::BlockId &) {
    return authorities_;
  }

}  // namespace kagome::runtime::dummy
