/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_METADATA_HPP
#define KAGOME_CORE_RUNTIME_WAVM_METADATA_HPP

#include "runtime/metadata.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmMetadata final: public Metadata {
   public:
    WavmMetadata(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<OpaqueMetadata> metadata(
        const boost::optional<primitives::BlockHash> &block_hash) override {
      return executor_->call<OpaqueMetadata>(
          "Metadata_metadata", block_hash);
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_METADATA_HPP
