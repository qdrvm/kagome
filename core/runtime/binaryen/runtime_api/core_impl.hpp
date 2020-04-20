/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP
#define CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/core.hpp"

namespace kagome::runtime::binaryen {

  class CoreImpl : public RuntimeApi, public Core {
   public:
    CoreImpl(const std::shared_ptr<RuntimeManager> &runtime_manager);

    ~CoreImpl() override = default;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block) override;

    outcome::result<void> initialise_block(
        const kagome::primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP
