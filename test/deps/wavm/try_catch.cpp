/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// See ../../../housekeeping/macos/libunwind/README.md

#ifndef WAVM_API
#define WAVM_API
#endif

#include <WAVM/IR/Module.h>
#include <WAVM/Runtime/Runtime.h>
#include <WAVM/RuntimeABI/RuntimeABI.h>
#include <WAVM/WASTParse/WASTParse.h>
#include <cassert>
#include <cstdio>

struct Wasm {
  static void unreachable() {
    WAVM::Runtime::throwException(
        WAVM::Runtime::ExceptionTypes::reachedUnreachable);
  }

  Wasm(const std::string &wast) {
    std::vector<WAVM::WAST::Error> wast_errors;
    if (not WAVM::WAST::parseModule(
            wast.data(), wast.size() + 1, module_ir_, wast_errors)) {
      WAVM::WAST::reportParseErrors("wast", wast.data(), wast_errors);
      abort();
    }
    module_ref_ = WAVM::Runtime::compileModule(module_ir_);
    assert(module_ref_);
    compartment_ = WAVM::Runtime::createCompartment();
    instance_ =
        WAVM::Runtime::instantiateModule(compartment_, module_ref_, {}, "");
    assert(instance_);
    fn_ = WAVM::Runtime::asFunction(
        WAVM::Runtime::getInstanceExport(instance_, "main"));
    assert(fn_);
    context_ = WAVM::Runtime::createContext(compartment_);
  }

  void call() {
    WAVM::Runtime::invokeFunction(context_, fn_);
  }

  WAVM::IR::Module module_ir_;
  WAVM::Runtime::ModuleRef module_ref_;
  WAVM::Runtime::Compartment *compartment_;
  WAVM::Runtime::Instance *instance_;
  WAVM::Runtime::Function *fn_;
  WAVM::Runtime::Context *context_;
};

int main() {
  // Can catch wavm unreachable exception?
  printf("unreachable: throw\n");
  try {
    Wasm::unreachable();
  } catch (WAVM::Runtime::Exception *) {
    printf("unreachable: catch\n");
  }

  // Can catch wavm unreachable exception from wasm call stack?
  // MacOS M1 libunwind crashes.
  Wasm wasm{R"wast(
    (module
      (func (export "main")
        unreachable
      )
    )
  )wast"};
  printf("wasm: call\n");
  try {
    wasm.call();
  } catch (WAVM::Runtime::Exception *) {
    printf("wasm: catch\n");
  }
}
