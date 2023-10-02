/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/core_api_factory.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class Executor;
  class RuntimeContextFactory;
  class ModuleFactory;
}  // namespace kagome::runtime

namespace kagome::runtime::wasm_edge {

  class CoreApiFactoryImpl final : public CoreApiFactory {
   public:
    CoreApiFactoryImpl(std::shared_ptr<const ModuleFactory> factory);

    [[nodiscard]] virtual outcome::result<std::unique_ptr<RestrictedCore>> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t> &runtime_code) const override;

   private:
    std::shared_ptr<const ModuleFactory> factory_;
  };

}  // namespace kagome::runtime::wasm_edge
