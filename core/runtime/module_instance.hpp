/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_MODULE_INSTANCE_HPP

#include <string_view>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "outcome/outcome.hpp"
#include "runtime/ptr_size.hpp"

namespace kagome::runtime {

  static_assert(sizeof(float) == 4);
  static_assert(sizeof(double) == 8);
  using WasmValue = boost::variant<int32_t, int64_t, float, double>;

  /**
   * An instance of a WebAssembly code module
   * Exposes a set of functions and global variables
   */
  class ModuleInstance {
   public:
    virtual ~ModuleInstance() = default;

    /**
     * Call the instance's function
     * @param name - name of the function
     * @param args - a pointer-size describing a buffer with the function parameters
     * @return a pointer-size with the buffer returned by the call
     */
    virtual outcome::result<PtrSize> callExportFunction(std::string_view name,
                                                        PtrSize args) const = 0;

    virtual outcome::result<boost::optional<WasmValue>> getGlobal(
        std::string_view name) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_INSTANCE_HPP
