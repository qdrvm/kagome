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

void test(std::string name, const kagome::runtime::ModuleFactory &factory) {
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
    auto wat_path =
        kagome::filesystem::absolute(__FILE__).parent_path().string()
        + "/wat/memory_fill.wat";
    std::string wat_code;
    EXPECT_TRUE(qtils::readFile(wat_code, wat_path));
    std::string_view wat_code_view(wat_code);
    auto code = kagome::runtime::watToWasm(kagome::str2byte(wat_code_view));
    auto instrument = std::make_shared<kagome::runtime::WasmInstrumenter>();
    auto instrumented = instrument->instrument(code, context_params);
    if (not instrumented) {
      fmt::println("instrument: [{}]", instrumented.error().message());
    }
    if (bulk) {
      EXPECT_TRUE(instrumented);
      code = std::move(instrumented.value());
    } else {
      EXPECT_FALSE(instrumented);
    }
    if (not instrumented) {
      return nullptr;
    }

    auto _compile = factory.compile(path, code, context_params);
    if (not _compile) {
      fmt::println("compile: [{}]", _compile.error().message());
    }
    if (bulk) {
      EXPECT_TRUE(_compile);
    } else {
      EXPECT_FALSE(_compile);
    }
    if (not _compile) {
      return nullptr;
    }
    auto _module = factory.loadCompiled(path, context_params);
    if (not _module) {
      fmt::println("loadCompiled: [{}]", _module.error().message());
    }
    if (bulk) {
      EXPECT_TRUE(_module);
    } else {
      EXPECT_FALSE(_module);
    }
    if (not _module) {
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

class WasmBulkMemoryTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    testutil::prepareLoggers();
    EXPECT_CALL(*trie_storage, getEphemeralBatchAt(_)).WillRepeatedly([] {
      return nullptr;
    });
  }

  static void TearDownTestSuite() {
    trie_storage.reset();
  }
};

TEST_F(WasmBulkMemoryTest, wasm_edge_compiler) {
  test("wasmedge-compile", *kagome::runtime::wasm_edge::compiler());
}

TEST_F(WasmBulkMemoryTest, wasm_edge_interpreter) {
  test("wasmedge-interpret", *kagome::runtime::wasm_edge::interpreter());
}
