/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

#include <boost/variant.hpp>
#include <optional>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/ptr_size.hpp"

namespace kagome::runtime {
  class Module;

  static_assert(sizeof(float) == 4);
  static_assert(sizeof(double) == 8);
  using WasmValue = boost::variant<int32_t, int64_t, float, double>;

  /**
   * An instance of a WebAssembly code module
   * Exposes a set of functions and global variables
   */
  class ModuleInstance {
   public:
    enum class Error {
      ABSENT_HEAP_BASE = 1,
      HEAP_BASE_TOO_LOW,
    };

    virtual ~ModuleInstance() = default;

    virtual const common::Hash256 &getCodeHash() const = 0;

    virtual std::shared_ptr<const Module> getModule() const = 0;

    /**
     * Call the instance's function
     * @param name - name of the function
     * @param args - a pointer-size describing a buffer with the function
     * parameters
     * @return a pointer-size with the buffer returned by the call
     */
    virtual outcome::result<PtrSize> callExportFunction(
        std::string_view name, common::BufferView encoded_args) const = 0;

    virtual outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name) const = 0;

    using SegmentOffset = size_t;
    using SegmentData = std::span<const uint8_t>;
    using DataSegmentProcessor =
        std::function<void(SegmentOffset, SegmentData)>;
    virtual void forDataSegment(const DataSegmentProcessor &callback) const = 0;

    virtual const InstanceEnvironment &getEnvironment() const = 0;
    virtual outcome::result<void> resetEnvironment() = 0;

    virtual outcome::result<void> resetMemory(const MemoryLimits &config);
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, ModuleInstance::Error);
