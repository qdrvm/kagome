/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_CORE_HPP
#define KAGOME_RUNTIME_WAVM_CORE_HPP

#include "runtime/core.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmCore final: public Core {
   public:
    WavmCore(std::shared_ptr<Executor> executor)
        : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<primitives::Version> version(
        const boost::optional<primitives::BlockHash> &block_hash) override {
      return executor_->call<primitives::Version>(
          "Core_version", block_hash);
    }

    outcome::result<void> execute_block(
        const primitives::Block &block) override {
      return executor_->call<void>(
          "Core_execute_block", block);
    }

    outcome::result<void> initialise_block(
        const primitives::BlockHeader &header) override {
      return executor_->call<void>(
          "Core_initialise_block", header);
    }

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override {
      return executor_->call<std::vector<primitives::AuthorityId>>(
          "Core_authorities", block_id);
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_CORE_HPP
