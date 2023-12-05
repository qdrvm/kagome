/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_instance.hpp"

#include "log/logger.hpp"

namespace wasm {
  using namespace ::wasm;  // NOLINT(google-build-using-namespace)
  class Module;
  class ModuleInstance;
}  // namespace wasm

namespace kagome::runtime::binaryen {
  class ModuleImpl;

  class RuntimeExternalInterface;

  class ModuleInstanceImpl final : public ModuleInstance {
   public:
    enum class Error {
      UNEXPECTED_EXIT = 1,
      EXECUTION_ERROR,
      CAN_NOT_OBTAIN_GLOBAL,
    };

    ModuleInstanceImpl(InstanceEnvironment &&env,
                       std::shared_ptr<const ModuleImpl> parent,
                       std::shared_ptr<RuntimeExternalInterface> rei,
                       const common::Hash256 &code_hash);

    const common::Hash256 &getCodeHash() const override {
      return code_hash_;
    }

    std::shared_ptr<const Module> getModule() const override;

    outcome::result<common::Buffer> callExportFunction(
        RuntimeContext &ctx,
        std::string_view name,
        common::BufferView args) const override;

    outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name) const override;

    const InstanceEnvironment &getEnvironment() const override;

    outcome::result<void> resetEnvironment() override;

    void forDataSegment(const DataSegmentProcessor &callback) const override;

   private:
    InstanceEnvironment env_;
    std::shared_ptr<RuntimeExternalInterface> rei_;
    std::shared_ptr<const ModuleImpl>
        parent_;  // must be kept alive because binaryen's module instance keeps
                  // a reference to it
    common::Hash256 code_hash_;

    std::unique_ptr<wasm::ModuleInstance> module_instance_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, ModuleInstanceImpl::Error);
