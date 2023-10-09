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

    ~BasicCodeProvider() override = default;

    outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &state) const override;

   private:
    void initialize(std::string_view path);

    kagome::common::Buffer buffer_;
  };

}  // namespace kagome::runtime
