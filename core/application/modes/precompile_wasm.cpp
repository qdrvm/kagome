/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/modes/precompile_wasm.hpp"

#include "blockchain/block_tree.hpp"
#include "common/bytestr.hpp"
#include "parachain/pvf/pool.hpp"
#include "parachain/pvf/session_params.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "utils/read_file.hpp"

namespace kagome::application::mode {
  PrecompileWasmMode::PrecompileWasmMode(
      const application::AppConfiguration &app_config,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<parachain::PvfPool> module_factory)
      : log_{log::createLogger("PrecompileWasm")},
        config_{*app_config.precompileWasm()},
        block_tree_{std::move(block_tree)},
        parachain_api_{std::move(parachain_api)},
        hasher_{std::move(hasher)},
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
      if (auto res = readFile(bytes, path); !res) {
        SL_ERROR(log_, "file {} read error {}", path.string(), res.error());
        continue;
      }
      auto text = byte2str(bytes);
      if (text.starts_with("{")) {
        SL_ERROR(log_, "expected WASM, got JSON, file {}", path.string());
        continue;
      }
      if (text.starts_with("0x")) {
        BOOST_OUTCOME_TRY(bytes, common::unhexWith0x(text));
      }
      // https://github.com/paritytech/polkadot-sdk/blob/b4ae5b01da280f754ccc00b94314a30b658182a1/polkadot/parachain/src/primitives.rs#L74-L81
      auto code_hash = hasher_->blake2b_256(bytes);
      OUTCOME_TRY(config,
                  parachain::sessionParams(*parachain_api_, block.hash));
      OUTCOME_TRY(module_factory_->precompile(code_hash, bytes, config));
    }
    return outcome::success();
  }
}  // namespace kagome::application::mode
