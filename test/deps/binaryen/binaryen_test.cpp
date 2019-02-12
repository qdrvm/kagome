/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/format.hpp>
#include <shell-interface.h>
#include <wasm-s-parser.h>

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
  explicit IntParamExternalInterface(std::string env_name, std::string fun_name,
                                     FunType &&f)
      : env_name_(env_name), fun_name_(fun_name), f_(std::move(f)) {}

  Literal callImport(Function *import, LiteralList &arguments) override {
    if (import->module == env_name_.c_str() &&
        import->base == fun_name_.c_str()) {
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
 * (foo) with given argument
 * @when this code is interpreted using Binaryen
 * @then foo implementation on C++ is invoked with given argument
 */
TEST(BinaryenTest, Example1) {
  int expected_argument = 1234;
  std::string env_name = "env";
  std::string fun_name = "foo";
  auto fun_impl = [&](int a) { ASSERT_EQ(a, expected_argument); };

  // wast code with imported function's call
  auto fmt = boost::format("(module\n"
                           " (type $v (func))\n"
                           " (import \"%1%\" \"%2%\" (func $%2% (param i32)))\n"
                           " (start $starter)\n"
                           " (func $starter (; 1 ;) (type $v)\n"
                           "  (call $%2%\n"
                           "   (i32.const %3%)\n"
                           "  )\n"
                           " )\n"
                           ")") %
             env_name % fun_name % expected_argument;
  std::string add_wast = fmt.str();

  // parse wast
  auto *wasm = new Module;
  SExpressionParser parser(const_cast<char *>(add_wast.c_str()));
  Element &root = *parser.root;
  SExpressionWasmBuilder builder(*wasm, *root[0]);

  // prepare external interface with imported function's implementation
  IntParamExternalInterface interface(env_name, fun_name, fun_impl);

  // interpret module
  ModuleInstance instance(*wasm, &interface);

  // dispose module
  delete wasm;
}
