/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_code_provider.hpp"

namespace kagome::runtime {

  /**
   * A code provider that serves only one fixed blob of code for any state
   */
  class ConstantCodeProvider final : public RuntimeCodeProvider {
   public:
    explicit ConstantCodeProvider(common::Buffer code);

    outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &at) const override;

   private:
    common::Buffer code_;
  };

}  // namespace kagome::runtime
