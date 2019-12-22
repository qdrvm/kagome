/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/grandpa.hpp"
#include "runtime/impl/runtime_api.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {
  class GrandpaImpl : public RuntimeApi, public Grandpa {
   public:
    GrandpaImpl(const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
                const std::shared_ptr<extensions::Extension> &extension);

    ~GrandpaImpl() override = default;

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<std::vector<WeightedAuthority>> authorities(
        const primitives::BlockId &block_id) override;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP
