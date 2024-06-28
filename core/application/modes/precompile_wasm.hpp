/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_configuration.hpp"
#include "application/mode.hpp"
#include "log/logger.hpp"

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::parachain {
  class PvfPool;
}  // namespace kagome::parachain

namespace kagome::runtime {
  class ParachainHost;
}  // namespace kagome::runtime

namespace kagome::application::mode {
  /**
   * Precompile WASM before tests to avoid compilation during tests.
   */
  class PrecompileWasmMode final : public Mode {
   public:
    PrecompileWasmMode(const application::AppConfiguration &app_config,
                       std::shared_ptr<blockchain::BlockTree> block_tree,
                       std::shared_ptr<runtime::ParachainHost> parachain_api,
                       std::shared_ptr<crypto::Hasher> hasher,
                       std::shared_ptr<parachain::PvfPool> module_factory);

    int run() const override;

   private:
    outcome::result<void> runOutcome() const;

    log::Logger log_;
    application::PrecompileWasmConfig config_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<parachain::PvfPool> module_factory_;
  };
}  // namespace kagome::application::mode
