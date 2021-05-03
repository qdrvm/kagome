/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "intrinsic_resolver.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "crutch.hpp"

#include "gc_compartment.hpp"

namespace kagome::runtime::wavm {

  static const auto kIntrinsicMemoryType{
      WAVM::IR::MemoryType(false, WAVM::IR::IndexType::i32, {20, UINT64_MAX})};

  IntrinsicResolver::IntrinsicResolver()
      : module_{getIntrinsicModule_env()},
        module_instance_{},
        memory_{std::make_shared<Memory>(WAVM::Runtime::createMemory(
            getCompartment(), kIntrinsicMemoryType, "stub memory"))},
        compartment_{getCompartment()} {
    BOOST_ASSERT(getIntrinsicModule_env() != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
  }

  bool IntrinsicResolver::resolve(const std::string &moduleName,
                                  const std::string &exportName,
                                  WAVM::IR::ExternType type,
                                  WAVM::Runtime::Object *&outObject) {
    if (module_instance_ == nullptr) {
      module_instance_ = WAVM::Intrinsics::instantiateModule(
          compartment_, {getIntrinsicModule_env()}, "env");
      memory_->setUnderlyingMemory(getTypedInstanceExport(
          module_instance_, "memory", kIntrinsicMemoryType));
    }

    if (moduleName != "env") {
      return false;
    }
    if (exportName == "memory") {
      if (type.kind == WAVM::IR::ExternKind::memory) {
        auto memory{getTypedInstanceExport(
            module_instance_, "memory", kIntrinsicMemoryType)};
        outObject = WAVM::Runtime::asObject(memory);
        return true;
      }
      return false;
    }
    if (type.kind == WAVM::IR::ExternKind::function) {
      std::string_view func_name{exportName};
      // cut off "ext_"
      // func_name = func_name.substr(4);
      // auto pos = func_name.find('_');
      // std::string_view category_name = func_name.substr(0, pos);
      // func_name = func_name.substr(pos + 1);

      auto f = functions_.find(func_name);
      if (f == functions_.end()) return false;
      auto func_type = f->second->getType();
      // discard 'intrinsic' calling convention
      WAVM::IR::FunctionType new_type{func_type.results(), func_type.params()};
      auto typed_export =
          getTypedInstanceExport(module_instance_, func_name.data(), new_type);
      if (asFunctionType(type) != new_type) {
        return false;
      }
      outObject = WAVM::Runtime::asObject(typed_export);
      return true;
    }
    return false;
  }

}  // namespace kagome::runtime::wavm
