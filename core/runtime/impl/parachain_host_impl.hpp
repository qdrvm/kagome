/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP

#include "runtime/parachain_host.hpp"

#include <outcome/outcome.hpp>
#include "primitives/scale_codec.hpp"
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  class ParachainHostImpl : public ParachainHost {
   public:
    ~ParachainHostImpl() override = default;

    /**
     * @brief constructor
     * @param state_code error or result code
     * @param extension extension instance
     * @param codec scale codec instance
     */
    ParachainHostImpl(common::Buffer state_code,
                      std::shared_ptr<extensions::Extension> extension,
                      std::shared_ptr<primitives::ScaleCodec> codec);

    outcome::result<DutyRoster> dutyRoster() override;

    outcome::result<std::vector<ParachainId>> activeParachains() override;

    outcome::result<std::optional<Buffer>> parachainHead(
        ParachainId id) override;

    outcome::result<std::optional<Buffer>> parachainCode(
        ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators(
        primitives::BlockId block_id) override;

   private:
    std::shared_ptr<WasmMemory> memory_;
    std::shared_ptr<primitives::ScaleCodec> codec_;
    WasmExecutor executor_;
    common::Buffer state_code_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_IMPL_HPP
