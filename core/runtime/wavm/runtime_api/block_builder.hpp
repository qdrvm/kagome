/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_BLOCK_BUILDER_HPP
#define KAGOME_RUNTIME_WAVM_BLOCK_BUILDER_HPP

#include "runtime/block_builder.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmBlockBuilder final: public BlockBuilder {
   public:
    WavmBlockBuilder(std::shared_ptr<Executor> executor)
        : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<primitives::ApplyResult> apply_extrinsic(
        const primitives::Extrinsic &extrinsic) override {
      return executor_->call<primitives::ApplyResult>(
          "BlockBuilder_apply_extrinsic", extrinsic);
    }

    outcome::result<primitives::BlockHeader> finalise_block() override {
      return executor_->call<primitives::BlockHeader>(
          "BlockBuilder_finalise_block");
    }

    outcome::result<std::vector<primitives::Extrinsic>>
    inherent_extrinsics(const primitives::InherentData &data) override {
      return executor_->call<std::vector<primitives::Extrinsic>>(
          "BlockBuilder_inherent_extrinsics", data);
    }

    outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) override {
      return executor_->call<primitives::CheckInherentsResult>(
          "BlockBuilder_check_inherents", block, data);
    }

    outcome::result<common::Hash256> random_seed() override {
      return executor_->call<common::Hash256>(
          "BlockBuilder_random_seed");
    }

   private:
    std::shared_ptr<Executor> executor_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_BLOCK_BUILDER_HPP
