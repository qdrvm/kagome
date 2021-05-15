/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_CODE_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_CODE_PROVIDER_HPP

#include <gsl/span>
#include <boost/optional.hpp>

#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  /**
   * @class RuntimeCodeProvider keeps and provides wasm state code
   */
  class RuntimeCodeProvider {
   public:
    virtual ~RuntimeCodeProvider() = default;

    struct CodeAndItsState {
      gsl::span<const uint8_t> code;
      storage::trie::RootHash state;
    };
    virtual outcome::result<CodeAndItsState> getCodeAt(
        const primitives::BlockInfo &at) const = 0;

    virtual outcome::result<CodeAndItsState> getLatestCode() const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_CODE_PROVIDER_HPP
