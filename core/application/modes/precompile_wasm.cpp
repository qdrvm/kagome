/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/modes/precompile_wasm.hpp"

#include "blockchain/block_tree.hpp"
#include "common/bytestr.hpp"
#include "parachain/pvf/session_params.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/wabt/instrument.hpp"
#include "utils/read_file.hpp"

namespace kagome::application::mode {
  PrecompileWasmMode::PrecompileWasmMode(
      const application::AppConfiguration &app_config,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<runtime::ModuleFactory> module_factory)
      : log_{log::createLogger("PrecompileWasm")},
        config_{*app_config.precompileWasm()},
        block_tree_{std::move(block_tree)},
        parachain_api_{std::move(parachain_api)},
        module_factory_{std::move(module_factory)} {}

  int PrecompileWasmMode::run() const {
    auto r = runOutcome();
    if (not r) {
      SL_ERROR(log_, "runOutcome: {}", r.error());
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  outcome::result<void> PrecompileWasmMode::runOutcome() const {
    auto block = block_tree_->bestBlock();

    // relay already precompiled, when getting version for genesis state

    for (auto &path : config_.parachains) {
      SL_INFO(log_, "precompile parachain {}", path.string());
      Buffer bytes;
      if (not readFile(bytes, path)) {
        SL_WARN(log_, "read error, file {}", path.string());
        continue;
      }
      auto text = byte2str(bytes);
      if (text.starts_with("{")) {
        SL_WARN(log_, "expected WASM, got JSON, file {}", path.string());
        continue;
      }
      if (text.starts_with("0x")) {
        BOOST_OUTCOME_TRY(bytes, common::unhexWith0x(text));
      }
      Buffer code;
      OUTCOME_TRY(runtime::uncompressCodeIfNeeded(bytes, code));
      OUTCOME_TRY(config,
                  parachain::sessionParams(*parachain_api_, block.hash));
      BOOST_OUTCOME_TRY(
          code, runtime::prepareBlobForCompilation(code, config.memory_limits));
      OUTCOME_TRY(module_factory_->make(code));
    }
    return outcome::success();
  }
}  // namespace kagome::application::mode
