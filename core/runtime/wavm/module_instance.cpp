/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_instance.hpp"

#include <WAVM/Runtime/Runtime.h>
#include <WAVM/RuntimeABI/RuntimeABI.h>

#include "host_api/host_api.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/memory_impl.hpp"
#include "runtime/wavm/module.hpp"

static WAVM::IR::Value evaluateInitializer(
    WAVM::IR::InitializerExpression expression) {
  using WAVM::IR::InitializerExpression;
  switch (expression.type) {
    case InitializerExpression::Type::i32_const:
      return expression.i32;
    case InitializerExpression::Type::i64_const:
      return expression.i64;
    case InitializerExpression::Type::f32_const:
      return expression.f32;
    case InitializerExpression::Type::f64_const:
      return expression.f64;
    case InitializerExpression::Type::v128_const:
      return expression.v128;
    case InitializerExpression::Type::global_get: {
      throw std::runtime_error{"Not implemented on WAVM yet"};
    }
    case InitializerExpression::Type::ref_null:
      return WAVM::IR::Value(asValueType(expression.nullReferenceType),
                             WAVM::IR::UntaggedValue());

    case InitializerExpression::Type::ref_func:
      // instantiateModule delays evaluating ref.func initializers until the
      // module is loaded and we have addresses for its functions.

    case InitializerExpression::Type::invalid:
    default:
      WAVM_UNREACHABLE();
  };
}

static WAVM::Uptr getIndexValue(const WAVM::IR::Value &value,
                                WAVM::IR::IndexType indexType) {
  switch (indexType) {
    case WAVM::IR::IndexType::i32:
      WAVM_ASSERT(value.type == WAVM::IR::ValueType::i32);
      return value.u32;
    case WAVM::IR::IndexType::i64:
      WAVM_ASSERT(value.type == WAVM::IR::ValueType::i64);
      return value.u64;
    default:
      WAVM_UNREACHABLE();
  };
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::wavm,
                            ModuleInstanceImpl::Error,
                            e) {
  using E = kagome::runtime::wavm::ModuleInstanceImpl::Error;
  switch (e) {
    case E::WRONG_ARG_COUNT:
      return "The provided function argument count should equal to 2";
    case E::EXECUTION_ERROR:
      return "An error occurred during wasm call execution; Check the logs for "
             "more information";
    case E::WRONG_RETURN_TYPE:
      return "Runtime function returned result of unsupported type";
  }
  return "Unknown error in ModuleInstance";
}

namespace kagome::runtime::wavm {

  ModuleInstanceImpl::ModuleInstanceImpl(
      InstanceEnvironment &&env,
      WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance,
      std::shared_ptr<const ModuleImpl> module,
      std::shared_ptr<const CompartmentWrapper> compartment,
      const common::Hash256 &code_hash)
      : env_{std::move(env)},
        instance_{std::move(instance)},
        module_{std::move(module)},
        compartment_{std::move(compartment)},
        code_hash_(code_hash),
        logger_{log::createLogger("ModuleInstance", "wavm")} {
    BOOST_ASSERT(instance_ != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(module_ != nullptr);
  }

  std::shared_ptr<const Module> ModuleInstanceImpl::getModule() const {
    return module_;
  }

  outcome::result<PtrSize> ModuleInstanceImpl::callExportFunction(
      std::string_view name, common::BufferView encoded_args) const {
    auto memory = env_.memory_provider->getCurrentMemory().value();

    PtrSize args_span{memory.get().storeBuffer(encoded_args)};

    auto res = [this, name, args_span]() -> outcome::result<PtrSize> {
      WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context =
          WAVM::Runtime::createContext(compartment_->getCompartment());
      WAVM::Runtime::Function *function = WAVM::Runtime::asFunctionNullable(
          WAVM::Runtime::getInstanceExport(instance_, name.data()));
      if (!function) {
        SL_DEBUG(logger_, "The requested function {} not found", name);
        return RuntimeTransactionError::EXPORT_FUNCTION_NOT_FOUND;
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
      WAVM_ASSERT(function)

      WAVM::IR::TypeTuple invokeArgTypes{WAVM::IR::ValueType::i32,
                                         WAVM::IR::ValueType::i32};
      // Infer the expected type of the function from the number and type of the
      // invocation arguments and the function's actual result types.
      const WAVM::IR::FunctionType invokeSig(
          WAVM::Runtime::getFunctionType(function).results(),
          std::move(invokeArgTypes));
      // Allocate an array to receive the invocation results.
      BOOST_ASSERT(invokeSig.results().size() == 1);
      std::array<WAVM::IR::UntaggedValue, 1> untaggedInvokeResults;
      pushBorrowedRuntimeInstance(
          std::const_pointer_cast<ModuleInstanceImpl>(shared_from_this()));
      const auto pop = gsl::finally(&popBorrowedRuntimeInstance);
      try {
        WAVM::Runtime::unwindSignalsAsExceptions(
            [&context,
             &function,
             &invokeSig,
             args = args_span,
             resultsDestination = untaggedInvokeResults.data()] {
              std::array<WAVM::IR::UntaggedValue, 2> untaggedInvokeArgs{
                  static_cast<WAVM::U32>(args.ptr),
                  static_cast<WAVM::U32>(args.size)};
              WAVM::Runtime::invokeFunction(context,
                                            function,
                                            invokeSig,
                                            untaggedInvokeArgs.data(),
                                            resultsDestination);
            });
        return PtrSize{untaggedInvokeResults[0].u64};
      } catch (WAVM::Runtime::Exception *e) {
        const auto desc = WAVM::Runtime::describeException(e);
        logger_->error(desc);
        WAVM::Runtime::destroyException(e);
        return Error::EXECUTION_ERROR;
      }
    }();
    WAVM::Runtime::collectCompartmentGarbage(compartment_->getCompartment());
    return res;
  }

  outcome::result<std::optional<WasmValue>> ModuleInstanceImpl::getGlobal(
      std::string_view name) const {
    auto global = WAVM::Runtime::asGlobalNullable(
        WAVM::Runtime::getInstanceExport(instance_, name.data()));
    if (global == nullptr) {
      return std::nullopt;
    }
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

  void ModuleInstanceImpl::forDataSegment(
      const DataSegmentProcessor &callback) const {
    using WAVM::Uptr;
    using WAVM::IR::DataSegment;
    using WAVM::IR::MemoryType;
    using WAVM::IR::Value;
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif
    auto &ir = getModuleIR(module_->module_);
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif

    for (Uptr segmentIndex = 0; segmentIndex < ir.dataSegments.size();
         ++segmentIndex) {
      const DataSegment &dataSegment = ir.dataSegments[segmentIndex];
      if (dataSegment.isActive) {
        const Value baseOffsetValue =
            evaluateInitializer(dataSegment.baseOffset);
        const MemoryType &memoryType =
            ir.memories.getType(dataSegment.memoryIndex);
        Uptr baseOffset = getIndexValue(baseOffsetValue, memoryType.indexType);
        callback(baseOffset, *dataSegment.data);
      }
    }
  }

  const InstanceEnvironment &ModuleInstanceImpl::getEnvironment() const {
    return env_;
  }

  outcome::result<void> ModuleInstanceImpl::resetEnvironment() {
    env_.host_api->reset();
    return outcome::success();
  }
}  // namespace kagome::runtime::wavm
