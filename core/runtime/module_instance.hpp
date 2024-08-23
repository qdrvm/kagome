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
#include "common/monadic_utils.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/ptr_size.hpp"

namespace kagome::runtime {
  class Module;
  class RuntimeContext;
  class Memory;

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
      INVALID_CALL_RESULT,
    };

    template <typename... Args>
    static outcome::result<common::Buffer> encodeArgs(const Args &...args) {
      if constexpr (sizeof...(args) > 0) {
        return common::map_result(::scale::encode(args...), [](auto &&vec) {
          return common::Buffer{vec};
        });
      }
      return outcome::success();
    }

    template <typename Result>
    static outcome::result<Result> decodedCall(
        [[maybe_unused]] std::string_view method_name,
        outcome::result<common::Buffer> &&result) {
      OUTCOME_TRY(value, std::move(result));
      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        Result t{};
        scale::ScaleDecoderStream s(value);
        try {
          s >> t;
          // Check whether the whole byte buffer was consumed
          if (s.hasMore(1)) {
            static auto logger = log::createLogger("Executor", "runtime");
            SL_ERROR(
                logger,
                "Runtime API call '{}' result size exceeds the size of the "
                "type to initialize {} (read {}, total size {})",
                method_name,
                typeid(Result).name(),
                s.currentIndex(),
                s.span().size_bytes());
            return outcome::failure(ModuleInstance::Error::INVALID_CALL_RESULT);
          }
          return outcome::success(std::move(t));
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      }
    }

    virtual ~ModuleInstance() = default;

    virtual common::Hash256 getCodeHash() const = 0;

    virtual std::shared_ptr<const Module> getModule() const = 0;

    /**
     * Call an export function
     * @param ctx - context of the call
     * @param name - name of the function
     * @param args - a pointer-size describing a buffer with the function
     * parameters
     * @return a pointer-size with the buffer returned by the call
     */
    virtual outcome::result<common::Buffer> callExportFunction(
        RuntimeContext &ctx,
        std::string_view name,
        common::BufferView encoded_args) const = 0;

    template <typename Res, typename... Args>
    outcome::result<Res> callAndDecodeExportFunction(RuntimeContext &ctx,
                                                     std::string_view name,
                                                     const Args &...args) {
      OUTCOME_TRY(args_buf, encodeArgs(args...));
      return decodedCall<Res>(name, callExportFunction(ctx, name, args_buf));
    }

    virtual outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name) const = 0;

    using SegmentOffset = size_t;
    using SegmentData = std::span<const uint8_t>;
    using DataSegmentProcessor =
        std::function<void(SegmentOffset, SegmentData)>;
    virtual void forDataSegment(const DataSegmentProcessor &callback) const = 0;

    virtual const InstanceEnvironment &getEnvironment() const = 0;
    virtual outcome::result<void> resetEnvironment() = 0;

    outcome::result<void> resetMemory();

    virtual outcome::result<void> stateless();
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, ModuleInstance::Error);
