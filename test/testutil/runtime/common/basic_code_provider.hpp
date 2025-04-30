/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_code_provider.hpp"

namespace kagome::runtime {

  class BasicCodeProvider final : public kagome::runtime::RuntimeCodeProvider {
   public:
    explicit BasicCodeProvider(std::string_view path);

    Result getCodeAt(const storage::trie::RootHash &state) const override;
    Result getPendingCodeAt(
        const storage::trie::RootHash &state) const override;

   private:
    void initialize(std::string_view path);

    Code buffer_;
  };

}  // namespace kagome::runtime
