/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP

#include "primitives/scheduled_change.hpp"
#include "runtime/grandpa.hpp"
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/wasm_memory.hpp"

#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  //  class GrandpaImpl : public Grandpa {
  //   public:
  //    ~GrandpaImpl() override = default;
  //
  //    /**
  //     * @param state_code error or result code
  //     * @param extension extension instance
  //     * @param codec scale codec instance
  //     */
  //    GrandpaImpl(common::Buffer state_code,
  //                std::shared_ptr<extensions::Extension> extension);
  //
  //    outcome::result<std::optional<ScheduledChange>> pending_change(
  //        const Digest &digest) override;
  //
  //    outcome::result<std::optional<ForcedChange>> forced_change(
  //        const Digest &digest) override;
  //
  //    outcome::result<std::vector<WeightedAuthority>> authorities() override;
  //
  //   private:
  //    std::shared_ptr<WasmMemory> memory_;
  //    WasmExecutor executor_;
  //    common::Buffer state_code_;
  //  };

  class GrandpaImpl : public RuntimeApi, public Grandpa {
   public:
    GrandpaImpl(common::Buffer state_code,
               std::shared_ptr<extensions::Extension> extension)
        : RuntimeApi(state_code, extension) {}

    virtual outcome::result<std::optional<ScheduledChange>> pending_change(
        const Digest &digest) override {
      return TypedExecutor<std::optional<ScheduledChange>>(this).execute(
          "GrandpaApi_grandpa_pending_change", digest);
    }

    outcome::result<std::optional<ForcedChange>> forced_change(
        const Digest &digest) override {
      return TypedExecutor<std::optional<ForcedChange>>(this).execute(
          "GrandpaApi_grandpa_forced_change", digest);
    }

    outcome::result<std::vector<WeightedAuthority>> authorities() override {
      return TypedExecutor<std::vector<WeightedAuthority>>(this).execute(
          "GrandpaApi_grandpa_authorities");
    }
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_GRANDPA_IMPL_HPP
