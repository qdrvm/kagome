/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_MODULE_INSTANCE_HPP

#include <string_view>

#include <boost/variant.hpp>
#include <optional>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module_repository.hpp"
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
     * @brief Wrapper type over sptr<ModuleInstance>. Allows to return instance
     * back to the ModuleInstancePool upon destruction of
     * BorrowedInstance.
     */
    class BorrowedInstance {
     public:
      using PoolReleaseFunction = std::function<void()>;

      BorrowedInstance(std::shared_ptr<ModuleInstance> instance,
                       PoolReleaseFunction cache_release = {})
          : instance_{std::move(instance)},
            cache_release_{std::move(cache_release)} {}
      ~BorrowedInstance() {
        if (cache_release_) {
          cache_release_();
        }
      }
      bool operator==(std::nullptr_t) {
        return instance_ == nullptr;
      }
      ModuleInstance *operator->() {
        return instance_.get();
      }

     private:
      std::shared_ptr<ModuleInstance> instance_;
      PoolReleaseFunction cache_release_;
    };

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
    using SegmentData = gsl::span<const uint8_t>;
    using DataSegmentProcessor =
        std::function<void(SegmentOffset, SegmentData)>;
    virtual void forDataSegment(DataSegmentProcessor const &callback) const = 0;

    virtual InstanceEnvironment const &getEnvironment() const = 0;
    virtual outcome::result<void> resetEnvironment() = 0;

    /**
     * @brief Make thread borrow a wrapped pointer to this instance with custom
     * deleter 'release'
     *
     * @param release - a deleter, that should be called upon thread destruction
     */
    virtual void borrow(BorrowedInstance::PoolReleaseFunction release) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_INSTANCE_HPP
