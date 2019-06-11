/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/grandpa.hpp"
#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  class GrandpaImpl : public RuntimeApi, public Grandpa {
   public:
    GrandpaImpl(common::Buffer state_code,
                std::shared_ptr<extensions::Extension> extension);

    ~GrandpaImpl() override = default;

    outcome::result<std::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<std::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<std::vector<WeightedAuthority>> authorities() override;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP
