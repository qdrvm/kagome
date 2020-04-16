/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP
#define KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP

#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {

  class BasicWasmProvider : public kagome::runtime::WasmProvider {
   public:
    explicit BasicWasmProvider(std::string_view path);

    ~BasicWasmProvider() override = default;

    const kagome::common::Buffer &getStateCode() const override;

   private:
    void initialize(std::string_view path);

    kagome::common::Buffer buffer_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP
