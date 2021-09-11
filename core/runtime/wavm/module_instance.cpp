/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_instance.hpp"

#include <WAVM/Runtime/Runtime.h>
#include <WAVM/RuntimeABI/RuntimeABI.h>

#include "host_api/host_api.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"

#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/memory_impl.hpp"
#include "runtime/wavm/module.hpp"

#include "log/profiling_logger.hpp"

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

  size_t ModuleInstance::Count = 0;

  ModuleInstance::ModuleInstance(
      InstanceEnvironment &&env,
      WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance,
      std::shared_ptr<const CompartmentWrapper> compartment)
      : env_{std::move(env)},
        instance_{std::move(instance)},
        compartment_{std::move(compartment)},
        logger_{log::createLogger("ModuleInstance", "wavm")} {
    BOOST_ASSERT(instance_ != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
    Count++;
  }

  ModuleInstance::~ModuleInstance() {
    Count--;
  }

  outcome::result<PtrSize> ModuleInstance::callExportFunction(
      std::string_view name, PtrSize args) const {
    SL_VERBOSE(logger_, "Runtime entities count:");
    SL_VERBOSE(logger_, "\tModules: {}", ModuleImpl::Count);
    SL_VERBOSE(logger_, "\tModule instances: {}", ModuleInstance::Count);
    SL_VERBOSE(logger_, "\tIntrinsic modules: {}", IntrinsicModule::Count);
    SL_VERBOSE(logger_,
               "\tIntrinsic module instances: {}",
               IntrinsicModuleInstance::Count);
    SL_VERBOSE(logger_, "\tCompartments: {}", CompartmentWrapper::Count);
    SL_VERBOSE(logger_, "\tMemories: {}", MemoryImpl::Count);
    SL_VERBOSE(logger_, "\tHost Apis: {}", getHostApisNum());

    auto res = [this, name, args]() -> outcome::result<PtrSize> {
      WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context =
          WAVM::Runtime::createContext(compartment_->getCompartment());
      WAVM::Runtime::Function *function = WAVM::Runtime::asFunctionNullable(
          WAVM::Runtime::getInstanceExport(instance_, name.data()));
      std::vector<WAVM::IR::Value> invokeArgs;
      if (!function) {
        SL_DEBUG(logger_, "The requested function {} not found", name);
        return Error::FUNC_NOT_FOUND;
      }
      const WAVM::IR::FunctionType functionType =
          WAVM::Runtime::getFunctionType(function);
      if (functionType.params().size() != 2) {  // address and size
        SL_DEBUG(
            logger_,
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
    }();
    WAVM::Runtime::collectCompartmentGarbage(compartment_->getCompartment());
    return res;
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
        SL_DEBUG(logger_,
                 "Runtime function returned result of unsupported type: {}",
                 asString(value));
        return Error::WRONG_RETURN_TYPE;
    }
  }

  InstanceEnvironment const &ModuleInstance::getEnvironment() const {
    return env_;
  }

  outcome::result<void> ModuleInstance::resetEnvironment() {
    env_.host_api->reset();
    return outcome::success();
  }

}  // namespace kagome::runtime::wavm
