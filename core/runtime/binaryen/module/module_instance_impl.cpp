/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// Enables binaryen debug mode: every executed WASM instruction is printed out
// using Indenter (defined below) It is a massive amount of information, so it
// should be turned on only for specific cases when we need to follow web
// assembly execution very precisely. #define WASM_INTERPRETER_DEBUG

#include "runtime/binaryen/module/module_instance_impl.hpp"

#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/memory_provider.hpp"

#include <binaryen/wasm-interpreter.h>

#ifdef WASM_INTERPRETER_DEBUG

namespace wasm {

  // Indenter is declared in binaryen headers, but the following members are
  // defined in its source files and only if WASM_INTERPRETER_DEBUG macro
  // definition is set, which it isn't at the time of binaryen compilation.
  // Therefore, to be able to use binaryen's debug mode we have to define
  // indenter implementation ourselves. This design is unclear to me, but it
  // does work.
  int Indenter::indentLevel = 0;

  std::vector<std::string> indents = []() {
    std::vector<std::string> indents;
    for (size_t i = 0; i < 512; i++) {
      indents.push_back(std::string(i, '-'));
    }
    return indents;
  }();

  Indenter::Indenter(const char *entry) : entryName(entry) {
    ++indentLevel;
  }

  Indenter::~Indenter() {
    print();
    std::cout << "exit " << entryName << '\n';
    --indentLevel;
  }

  void Indenter::print() {
    std::cout << indentLevel << ':' << indents[indentLevel];
  }
}  // namespace wasm
#endif

#include <binaryen/wasm.h>

#include "host_api/host_api.hpp"
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
      InstanceEnvironment &&env,
      std::shared_ptr<const ModuleImpl> parent,
      std::shared_ptr<RuntimeExternalInterface> rei,
      const common::Hash256 &code_hash)
      : env_{std::move(env)},
        rei_{std::move(rei)},
        parent_{std::move(parent)},
        code_hash_(code_hash),
        module_instance_{std::make_unique<wasm::ModuleInstance>(
            *parent_->module_, rei_.get())},
        logger_{log::createLogger("ModuleInstance", "binaryen")} {
    BOOST_ASSERT(parent_);
    BOOST_ASSERT(module_instance_);
    BOOST_ASSERT(rei_);
  }

  std::shared_ptr<const Module> ModuleInstanceImpl::getModule() const {
    return parent_;
  }

  outcome::result<PtrSize> ModuleInstanceImpl::callExportFunction(
      std::string_view name, common::BufferView encoded_args) const {
    auto memory = env_.memory_provider->getCurrentMemory().value();

    PtrSize args{memory.get().storeBuffer(encoded_args)};

    const auto args_list =
        wasm::LiteralList{wasm::Literal{args.ptr}, wasm::Literal{args.size}};

    if (auto res =
            module_instance_->wasm.getExportOrNull(wasm::Name{name.data()});
        nullptr == res) {
      SL_DEBUG(logger_, "The requested function {} not found", name);
      return RuntimeTransactionError::EXPORT_FUNCTION_NOT_FOUND;
    }

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

  outcome::result<std::optional<WasmValue>> ModuleInstanceImpl::getGlobal(
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
          logger_->error(
              "Runtime function returned result of unsupported type: {}",
              wasm::printType(val.type));
          return std::nullopt;
      }
    } catch (wasm::TrapException &e) {
      return Error::CAN_NOT_OBTAIN_GLOBAL;
    }
  }

  const InstanceEnvironment &ModuleInstanceImpl::getEnvironment() const {
    return env_;
  }

  outcome::result<void> ModuleInstanceImpl::resetEnvironment() {
    env_.host_api->reset();
    return outcome::success();
  }

  void ModuleInstanceImpl::forDataSegment(
      const DataSegmentProcessor &callback) const {
    for (auto &segment : parent_->module_->memory.segments) {
      wasm::Address offset =
          (uint32_t)wasm::ConstantExpressionRunner<wasm::TrivialGlobalManager>(
              module_instance_->globals)
              .visit(segment.offset)
              .value.geti32();
      if (offset + segment.data.size()
          > parent_->module_->memory.initial * wasm::Memory::kPageSize) {
        throw std::runtime_error("invalid offset when initializing memory");
      }
      callback(offset,
               gsl::span<const uint8_t>(
                   reinterpret_cast<const uint8_t *>(segment.data.data()),
                   segment.data.size()));
    }
  }

}  // namespace kagome::runtime::binaryen
