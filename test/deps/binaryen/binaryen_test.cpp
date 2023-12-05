/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <binaryen/shell-interface.h>
#include <binaryen/wasm-s-parser.h>
#include <boost/format.hpp>
#include <utility>
using namespace wasm;

/**
 * An external interface with custom imported function. Should be used for
 * testing purposes
 */
class IntParamExternalInterface : public ShellExternalInterface {
 private:
  std::string env_name_;
  std::string fun_name_;

  using FunType = std::function<void(int)>;
  FunType f_;

 public:
  explicit IntParamExternalInterface(std::string env_name,
                                     std::string fun_name,
                                     FunType &&f)
      : env_name_(std::move(env_name)),
        fun_name_(std::move(fun_name)),
        f_(std::move(f)) {}

  Literal callImport(Function *import, LiteralList &arguments) override {
    if (import->module == env_name_.c_str()
        && import->base == fun_name_.c_str()) {
      if (arguments.size() != 1) {
        Fatal() << fun_name_ << "expected exactly 1 parameter";
      }
      auto arg = *arguments.begin();
      f_(arg.geti32());
      return Literal();
    }
    Fatal() << "callImport: unknown import: " << import->module.str << "."
            << import->name.str;
  }
};

/**
 * @given WebAssembly S-expression code with invocation of imported function
 * (foo) with given argument is implemented in C++
 * @when this code is interpreted using Binaryen
 * @then foo implementation on C++ is invoked with given argument
 */
TEST(BinaryenTest, InvokeCppFunctionFromWebAssembly) {
  int expected_argument = 1234;
  std::string env_name = "env";
  std::string fun_name = "foo";
  auto fun_impl = [&](int a) { ASSERT_EQ(a, expected_argument); };

  // wast code with imported function's call
  auto fmt = boost::format(
                 R"#(
              (module
                (type $v (func))
                (import "%1%" "%2%" (func $%2% (param i32)))
                (start $starter)
                (func $starter (; 1 ;) (type $v)
                  (call $%2%
                    (i32.const %3%)
                  )
                )
              )
              )#")
           % env_name % fun_name % expected_argument;

  std::string expression = fmt.str();

  // parse wast
  Module wasm{};

  // clang-8 doesn't know char * std::string::data(),
  // it returns only const char *
  char *data = const_cast<char *>(expression.data());
  SExpressionParser parser(data);
  Element &root = *parser.root;
  SExpressionWasmBuilder builder(wasm, *root[0]);

  // prepare external interface with imported function's implementation
  IntParamExternalInterface interface(env_name, fun_name, fun_impl);

  // interpret module
  ModuleInstance instance(wasm, &interface);
}

/**
 * @given WebAssembly S-expression code exporting a function
 * exported function (sumtwo) taking two arguments of type i32 returning their
 * sum of type i32 is implemented in assembly
 * @when this code is interpreted using Binaryen and invoked from C++ with given
 * arguments
 * @then result (the sum of two i32) is calculated and returned to C++ code
 */
TEST(BinaryenTest, InvokeWebAssemblyFunctionFromCpp) {
  // wast code with imported function's call
  char sexpr[] = R"#(
          (module
            (type $t0 (func (param i32 i32) (result i32)))
            (export "sumtwo" (func $sumtwo))
            (func $sumtwo (; 1 ;) (type $t0) (param $p0 i32) (param $p1 i32) (result i32)
              (i32.add
                (local.get $p0)
                (local.get $p1)
              )
            )
          )
          )#";

  // parse wast
  SExpressionParser parser(sexpr);
  auto &root = *parser.root;

  // wasm
  Module wasm{};

  // build wasm module
  SExpressionWasmBuilder builder(wasm, *root[0]);

  // interface
  ShellExternalInterface shellInterface;

  // interpret module
  ModuleInstance moduleInstance(wasm, &shellInterface);

  // add arguments, their constructors are explicit, so no list-initialization
  LiteralList arguments = {Literal{1}, Literal{2}};

  // call exported function
  Literal result = moduleInstance.callExport("sumtwo", arguments);

  ASSERT_EQ(result.type, Type::i32);
  ASSERT_EQ(result.geti32(), 3);
}
