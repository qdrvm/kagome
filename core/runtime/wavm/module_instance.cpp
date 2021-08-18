/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_instance.hpp"

#include <WAVM/Runtime/Runtime.h>
#include <WAVM/RuntimeABI/RuntimeABI.h>

#include "compartment_wrapper.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::wavm, ModuleInstance::Error, e) {
  using E = kagome::runtime::wavm::ModuleInstance::Error;
  switch (e) {
    case E::WRONG_ARG_COUNT:
      return "The provided function argument count should equal to 2";
    case E::FUNC_NOT_FOUND:
      return "The requested function not found";
    case E::EXECUTION_ERROR:
      return "An error occurred during wasm call execution; Check the logs for "
             "more information";
    case E::WRONG_RETURN_TYPE:
      return "Runtime function returned result of unsupported type";
  }
  return "Unknown error in ModuleInstance";
}

namespace kagome::runtime::wavm {

  ModuleInstance::ModuleInstance(
      WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance,
      std::shared_ptr<const CompartmentWrapper> compartment)
      : instance_{std::move(instance)},
        compartment_{std::move(compartment)},
        logger_{log::createLogger("ModuleInstance", "wavm")} {
    BOOST_ASSERT(instance_ != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
  }

  ModuleInstance::~ModuleInstance() {
    WAVM::Runtime::collectCompartmentGarbage(compartment_->getCompartment());
  }

  outcome::result<PtrSize> ModuleInstance::callExportFunction(
      std::string_view name, PtrSize args) const {
    WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context =
        WAVM::Runtime::createContext(compartment_->getCompartment());
    WAVM::Runtime::Function *function = WAVM::Runtime::asFunctionNullable(
        WAVM::Runtime::getInstanceExport(instance_, name.data()));
    std::vector<WAVM::IR::Value> invokeArgs;
    if (!function) {
      logger_->debug("The requested function {} not found", name);
      return Error::FUNC_NOT_FOUND;
    }
    const WAVM::IR::FunctionType functionType =
        WAVM::Runtime::getFunctionType(function);
    if (functionType.params().size() != 2) {  // address and size
      logger_->debug(
          "The provided function argument count should equal to 2, got {} "
          "instead",
          functionType.params().size());
      return Error::WRONG_ARG_COUNT;
    }
    invokeArgs.emplace_back(static_cast<WAVM::U32>(args.ptr));
    invokeArgs.emplace_back(static_cast<WAVM::U32>(args.size));
    WAVM_ASSERT(function)

    std::vector<WAVM::IR::ValueType> invokeArgTypes;
    std::vector<WAVM::IR::UntaggedValue> untaggedInvokeArgs;
    for (const WAVM::IR::Value &arg : invokeArgs) {
      invokeArgTypes.push_back(arg.type);
      untaggedInvokeArgs.push_back(arg);
    }
    // Infer the expected type of the function from the number and type of the
    // invoke arguments and the function's actual result types.
    const WAVM::IR::FunctionType invokeSig(
        WAVM::Runtime::getFunctionType(function).results(),
        WAVM::IR::TypeTuple(invokeArgTypes));
    // Allocate an array to receive the invoke results.
    std::vector<WAVM::IR::UntaggedValue> untaggedInvokeResults;
    untaggedInvokeResults.resize(invokeSig.results().size());
    try {
      WAVM::Runtime::unwindSignalsAsExceptions([&context,
                                                &function,
                                                &invokeSig,
                                                &untaggedInvokeArgs,
                                                &untaggedInvokeResults] {
        WAVM::Runtime::invokeFunction(context,
                                      function,
                                      invokeSig,
                                      untaggedInvokeArgs.data(),
                                      untaggedInvokeResults.data());
      });
      return PtrSize{untaggedInvokeResults[0].u64};
    } catch (WAVM::Runtime::Exception *e) {
      const auto desc = WAVM::Runtime::describeException(e);
      logger_->debug(desc);
      WAVM::Runtime::destroyException(e);
      return Error::EXECUTION_ERROR;
    }
  }

  outcome::result<boost::optional<WasmValue>> ModuleInstance::getGlobal(
      std::string_view name) const {
    auto global = WAVM::Runtime::asGlobalNullable(
        WAVM::Runtime::getInstanceExport(instance_, name.data()));
    if (global == nullptr) return boost::none;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context =
        WAVM::Runtime::createContext(compartment_->getCompartment());
    auto value = WAVM::Runtime::getGlobalValue(context, global);
    switch (value.type) {
      case WAVM::IR::ValueType::i32:
        return WasmValue{static_cast<int32_t>(value.i32)};
      case WAVM::IR::ValueType::i64:
        return WasmValue{static_cast<int64_t>(value.i64)};
      case WAVM::IR::ValueType::f32:
        return WasmValue{static_cast<float>(value.f32)};
      case WAVM::IR::ValueType::f64:
        return WasmValue{static_cast<double>(value.f64)};
      default:
        logger_->debug(
            "Runtime function returned result of unsupported type: {}",
            asString(value));
        return Error::WRONG_RETURN_TYPE;
    }
  }

}  // namespace kagome::runtime::wavm
