/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_GRANDPA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_GRANDPA_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/grandpa.hpp"

namespace kagome::runtime::binaryen {

  class GrandpaImpl : public RuntimeApi, public Grandpa {
   public:
    explicit GrandpaImpl(
        const std::shared_ptr<WasmProvider> &wasm_provider,
        const std::shared_ptr<RuntimeManager> &runtime_manager);

    ~GrandpaImpl() override = default;

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<primitives::AuthorityList> authorities(
        const primitives::BlockId &block_id) override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_GRANDPA_IMPL_HPP
