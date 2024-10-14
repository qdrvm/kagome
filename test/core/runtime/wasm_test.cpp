#include <qtils/hex.hpp>
#include <qtils/read_file.hpp>
#include "common/bytestr.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "host_api/host_api_factory.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wabt/instrument.hpp"
#include "runtime/wabt/util.hpp"
#include "runtime/wasm_edge/module_factory_impl.hpp"
#include "testutil/prepare_loggers.hpp"

#include <wabt/binary-reader-ir.h>
#include <wabt/binary-reader.h>
#include <wabt/binary-writer.h>
#include <wabt/ir.h>
#include <wabt/stream.h>
#include <wabt/wast-lexer.h>
#include <wabt/wast-parser.h>
#include <wabt/wat-writer.h>

using testing::_;

using kagome::runtime::wabtDecode;
using kagome::runtime::wabtEncode;

auto trie_storage = std::make_shared<kagome::storage::trie::TrieStorageMock>();
auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
struct StubHostApiFactory : kagome::host_api::HostApiFactory {
  std::unique_ptr<kagome::host_api::HostApi> make(
      std::shared_ptr<const kagome::runtime::CoreApiFactory>,
      std::shared_ptr<const kagome::runtime::MemoryProvider>,
      std::shared_ptr<kagome::runtime::TrieStorageProvider>) const override {
    return std::make_unique<kagome::host_api::HostApiMock>();
  }
};
auto host_api_factory = std::make_shared<StubHostApiFactory>();

namespace kagome::runtime::wasm_edge {
  auto make(bool compile) {
    using M = ModuleFactoryImpl;
    using E = M::ExecType;
    return std::make_shared<M>(hasher,
                               host_api_factory,
                               trie_storage,
                               nullptr,
                               nullptr,
                               compile ? E::Compiled : E::Interpreted);
  }
  auto interpreter = [] { return make(false); };
  auto compiler = [] { return make(true); };
}  // namespace kagome::runtime::wasm_edge

std::unique_ptr<wabt::Module> wat_to_module(std::span<const uint8_t> wat) {
  wabt::Result result;
  wabt::Errors errors;
  std::unique_ptr<wabt::WastLexer> lexer =
      wabt::WastLexer::CreateBufferLexer("", wat.data(), wat.size(), &errors);
  if (Failed(result)) {
    throw std::runtime_error{"Failed to parse WAT"};
  }

  std::unique_ptr<wabt::Module> module;
  wabt::WastParseOptions parse_wast_options{{}};
  result =
      wabt::ParseWatModule(lexer.get(), &module, &errors, &parse_wast_options);
  if (Failed(result)) {
    throw std::runtime_error{"Failed to parse module"};
  }
  return module;
}

std::vector<uint8_t> wat_to_wasm(std::span<const uint8_t> wat) {
  auto module = wat_to_module(wat);
  wabt::MemoryStream stream;
  if (wabt::Failed(wabt::WriteBinaryModule(
          &stream,
          module.get(),
          wabt::WriteBinaryOptions{{}, true, false, true}))) {
    throw std::runtime_error{"Failed to write binary wasm"};
  }
  return std::move(stream.output_buffer().data);
}

auto fromWat(std::string_view wat) {
  return wat_to_wasm(kagome::str2byte(wat));
}

void test(std::string name,
          const kagome::runtime::ModuleFactory &factory,
          bool is_interpreter = false) {
  using Instance = std::shared_ptr<kagome::runtime::ModuleInstance>;
  auto read = [&](Instance instance) {
    return fmt::format("{:x}",
                       instance->getEnvironment()
                           .memory_provider.get()
                           ->getCurrentMemory()
                           .value()
                           .get()
                           .view(0, 4)
                           .value());
  };
  auto compile = [&](bool bulk) -> Instance {
    auto path = fmt::format("test-wasm-{}-{}", name, bulk);
    auto context_params = kagome::runtime::RuntimeContext::ContextParams();
    context_params.wasm_ext_bulk_memory = bulk;
    if (not is_interpreter) {
      auto wat_path = kagome::filesystem::path(__FILE__).parent_path().string()
                    + "/wat/memory_fill.wat";
      std::string wat_code;
      EXPECT_TRUE(qtils::readFile(wat_code, wat_path));
      auto _compile = factory.compile(path, fromWat(wat_code), context_params);
      if (not _compile) {
        fmt::println("compile: [{}]", _compile.error().message());
      }
      if (bulk) {
        EXPECT_TRUE(_compile);
      } else {
        EXPECT_FALSE(_compile);
        return nullptr;
      }
    } else {
      path = kagome::filesystem::path(__FILE__).parent_path().string()
           + "/wasm/memory_fill.wasm";
    }
    auto _module = factory.loadCompiled(path, context_params);
    if (not _module) {
      fmt::println("loadCompiled: [{}]", _module.error().message());
    }
    if (bulk) {
      EXPECT_TRUE(_module);
    } else {
      EXPECT_FALSE(_module);
      return nullptr;
    }
    auto instance = _module.value()->instantiate();
    EXPECT_TRUE(instance);
    return instance.value();
  };
  auto test = [&](bool bulk) {
    fmt::println("{} bulk={}", name, bulk);
    auto instance = compile(bulk);
    if (not instance) {
      return;
    }
    auto ctx =
        kagome::runtime::RuntimeContextFactory::stateless(instance).value();
    EXPECT_EQ(read(instance), "00000000");
    instance->callExportFunction(ctx, "test", {}).value();
    EXPECT_EQ(read(instance), "01010101");
  };
  test(false);
  test(true);
}

int main() {
  testutil::prepareLoggers();
  EXPECT_CALL(*trie_storage, getEphemeralBatchAt(_)).WillRepeatedly([] {
    return nullptr;
  });

  test("wasmedge-interpret", *kagome::runtime::wasm_edge::interpreter(), true);
  test("wasmedge-compile", *kagome::runtime::wasm_edge::compiler(), false);

  trie_storage.reset();
}
