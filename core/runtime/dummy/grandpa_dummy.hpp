/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_FAKE_GRANDPA_DUMMY_HPP
#define KAGOME_CORE_RUNTIME_FAKE_GRANDPA_DUMMY_HPP

#include "application/key_storage.hpp"
#include "runtime/grandpa.hpp"

namespace kagome::runtime::dummy {
  class GrandpaDummy : public Grandpa {
   public:
    ~GrandpaDummy() override = default;

    explicit GrandpaDummy(std::shared_ptr<application::KeyStorage> key_storage);

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<std::vector<WeightedAuthority>> authorities(
        const primitives::BlockId &block_id) override;

   private:
    std::shared_ptr<application::KeyStorage> key_storage_;
  };
}  // namespace kagome::runtime::dummy

#endif  // KAGOME_CORE_RUNTIME_FAKE_GRANDPA_DUMMY_HPP
