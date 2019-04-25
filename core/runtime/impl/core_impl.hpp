/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_RUNTIME_CORE_IMPL_HPP
#define CORE_RUNTIME_CORE_IMPL_HPP

#include "common/logger.hpp"
#include "extensions/extension.hpp"
#include "primitives/scale_codec.hpp"
#include "runtime/core.hpp"
#include "runtime/impl/wasm_executor.hpp"

namespace kagome::runtime {

  class CoreImpl : public Core {
   public:
    CoreImpl(common::Buffer state_code,
             std::shared_ptr<extensions::Extension> extension,
             std::shared_ptr<primitives::ScaleCodec> codec);

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const kagome::primitives::Block &block) override;

    outcome::result<void> initialise_block(
        const kagome::primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        primitives::BlockId block_id) override;

   private:
    common::Buffer state_code_;
    std::shared_ptr<WasmMemory> memory_;
    WasmExecutor executor_;
    std::shared_ptr<primitives::ScaleCodec> codec_;
  };

}  // namespace kagome::runtime

#endif  // CORE_RUNTIME_CORE_IMPL_HPP
