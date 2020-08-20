/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/core_factory_impl.hpp"

#include "runtime/binaryen/runtime_api/core_impl.hpp"
#include "runtime/common/const_wasm_provider.hpp"

namespace kagome::runtime::binaryen {

  CoreFactoryImpl::CoreFactoryImpl(
      std::shared_ptr<RuntimeManager> runtime_manager,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : runtime_manager_{std::move(runtime_manager)},
        changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(runtime_manager_);
    BOOST_ASSERT(changes_tracker_);
    BOOST_ASSERT(header_repo_);
  }

  std::unique_ptr<Core> CoreFactoryImpl::createWithCode(
      std::shared_ptr<WasmProvider> wasm_provider) {
    return std::make_unique<CoreImpl>(std::move(wasm_provider),
                                      runtime_manager_,
                                      changes_tracker_,
                                      header_repo_);
  }

}  // namespace kagome::runtime::binaryen
