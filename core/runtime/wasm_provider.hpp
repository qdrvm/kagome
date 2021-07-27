/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_WASM_PROVIDER_HPP

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  /**
   * @class WasmProvider keeps and provides wasm state code
   */
  class WasmProvider {
   public:
    virtual ~WasmProvider() = default;

    virtual const common::Buffer &getStateCodeAt(
        const storage::trie::RootHash &at) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WASM_PROVIDER_HPP
