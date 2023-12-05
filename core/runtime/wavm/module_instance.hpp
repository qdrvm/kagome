/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_instance.hpp"

#include <string_view>

#include <WAVM/Runtime/Runtime.h>
#include <optional>

#include "log/logger.hpp"
#include "runtime/ptr_size.hpp"

namespace WAVM {
  namespace Runtime {
    struct Instance;
  }  // namespace Runtime
}  // namespace WAVM

namespace kagome::runtime::wavm {
  class ModuleImpl;

  class CompartmentWrapper;

  class ModuleInstanceImpl
      : public ModuleInstance,
        public std::enable_shared_from_this<ModuleInstanceImpl> {
   public:
    enum class Error {
      WRONG_ARG_COUNT = 1,
      EXECUTION_ERROR,
      WRONG_RETURN_TYPE
    };

    ModuleInstanceImpl(
        InstanceEnvironment &&env,
        WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance,
        std::shared_ptr<const ModuleImpl> module,
        std::shared_ptr<const CompartmentWrapper> compartment,
        const common::Hash256 &code_hash);

    const common::Hash256 &getCodeHash() const override {
      return code_hash_;
    }

    std::shared_ptr<const Module> getModule() const override;

    outcome::result<common::Buffer> callExportFunction(
        kagome::runtime::RuntimeContext &,
        std::string_view name,
        common::BufferView encoded_args) const override;

    outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name) const override;

    void forDataSegment(const DataSegmentProcessor &callback) const override;

    const InstanceEnvironment &getEnvironment() const override;
    outcome::result<void> resetEnvironment() override;

   private:
    InstanceEnvironment env_;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance_;
    std::shared_ptr<const ModuleImpl> module_;
    std::shared_ptr<const CompartmentWrapper> compartment_;
    common::Hash256 code_hash_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::wavm, ModuleInstanceImpl::Error)
