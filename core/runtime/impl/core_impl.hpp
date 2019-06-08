/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_RUNTIME_CORE_IMPL_HPP
#define CORE_RUNTIME_CORE_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/core.hpp"

namespace kagome::runtime {

  class CoreImpl : public Core {
   public:
    CoreImpl(common::Buffer state_code,
             std::shared_ptr<extensions::Extension> extension);

    ~CoreImpl() override;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const kagome::primitives::Block &block) override;

    outcome::result<void> initialise_block(
        const kagome::primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities()
        override;

   private:
    std::unique_ptr<RuntimeApi> runtime_;
  };

}  // namespace kagome::runtime

#endif  // CORE_RUNTIME_CORE_IMPL_HPP
