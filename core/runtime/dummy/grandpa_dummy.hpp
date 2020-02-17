/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_DUMMY_GRANDPA_DUMMY_HPP
#define KAGOME_CORE_RUNTIME_DUMMY_GRANDPA_DUMMY_HPP

#include "runtime/grandpa.hpp"

namespace kagome::runtime::dummy {

  /**
   * Dummy implementation of Grandpa runtime entries
   * Accepts the list of weighted authorities and returns it when requested
   */
  class GrandpaDummy : public Grandpa {
   public:
    explicit GrandpaDummy(std::vector<WeightedAuthority> authorities);

    ~GrandpaDummy() override = default;

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<std::vector<WeightedAuthority>> authorities(
        const primitives::BlockId &block_id) override;

   private:
    std::vector<WeightedAuthority> authorities_;
  };

}  // namespace kagome::runtime::dummy

#endif  // KAGOME_CORE_RUNTIME_DUMMY_GRANDPA_DUMMY_HPP
