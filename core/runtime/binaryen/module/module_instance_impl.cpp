/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_instance_impl.hpp"

#include <binaryen/wasm-interpreter.h>
#include <binaryen/wasm.h>

#include "runtime/binaryen/runtime_external_interface.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            ModuleInstanceImpl::Error,
                            e) {
  using kagome::runtime::binaryen::ModuleInstanceImpl;
  switch (e) {
    case ModuleInstanceImpl::Error::UNEXPECTED_EXIT:
      return "Execution was ended in external function";
    case ModuleInstanceImpl::Error::EXECUTION_ERROR:
      return "An error occurred during an export call execution";
    case ModuleInstanceImpl::Error::CAN_NOT_OBTAIN_GLOBAL:
      return "Failed to obtain a global value";
  }
  return "Unknown ModuleInstance error";
}

namespace kagome::runtime::binaryen {

  ModuleInstanceImpl::ModuleInstanceImpl(
      std::shared_ptr<wasm::Module> parent,
      const std::shared_ptr<RuntimeExternalInterface> &rei)
      : parent_{std::move(parent)},
        module_instance_{
            std::make_unique<wasm::ModuleInstance>(*parent_, rei.get())},
        logger_{log::createLogger("ModuleInstance", "binaryen")} {
    BOOST_ASSERT(parent_);
    BOOST_ASSERT(module_instance_);
  }

  outcome::result<PtrSize> ModuleInstanceImpl::callExportFunction(
      std::string_view name, PtrSize args) const {
    const auto args_list =
        wasm::LiteralList{wasm::Literal{args.ptr}, wasm::Literal{args.size}};
    try {
      const auto res = static_cast<uint64_t>(
          module_instance_->callExport(wasm::Name{name.data()}, args_list)
              .geti64());
      return PtrSize{res};
    } catch (wasm::ExitException &e) {
      return Error::UNEXPECTED_EXIT;
    } catch (wasm::TrapException &e) {
      return Error::EXECUTION_ERROR;
    }
  }

  outcome::result<boost::optional<WasmValue>> ModuleInstanceImpl::getGlobal(
      std::string_view name) const {
    try {
      auto val = module_instance_->getExport(name.data());
      switch (val.type) {
        case wasm::Type::i32:
          return WasmValue{val.geti32()};
        case wasm::Type::i64:
          return WasmValue{val.geti64()};
        case wasm::Type::f32:
          return WasmValue{val.getf32()};
        case wasm::Type::f64:
          return WasmValue{val.geti64()};
        default:
          logger_->debug(
              "Runtime function returned result of unsupported type: {}",
              wasm::printType(val.type));
          return boost::none;
      }
    } catch (wasm::TrapException &e) {
      return Error::CAN_NOT_OBTAIN_GLOBAL;
    }
  }

}  // namespace kagome::runtime::binaryen
