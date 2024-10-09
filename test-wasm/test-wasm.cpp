#include "crypto/hasher/hasher_impl.hpp"
#include "host_api/host_api_factory.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wasm_edge/module_factory_impl.hpp"
#include "testutil/prepare_loggers.hpp"

#include "wasm-bulk.hpp"

#include <qtils/hex.hpp>
#include <qtils/read_file.hpp>

using testing::_;

auto wasm = qtils::readBytes("test-wasm.wasm").value();

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

namespace kagome::runtime::binaryen {
  auto env_factory = std::make_shared<InstanceEnvironmentFactory>(
      trie_storage, nullptr, nullptr, host_api_factory);
  auto compiler =
      std::make_shared<ModuleFactoryImpl>(env_factory, trie_storage, hasher);
  void reset() {
    env_factory.reset();
    compiler.reset();
  }
}  // namespace kagome::runtime::binaryen

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
    wasmBulk() = bulk;
    auto path = fmt::format("test-wasm-{}-{}", name, bulk);
    auto _compile = factory.compile(path, wasm);
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
    // Q: should "compile" fail before "loadCompiled",
    //   e.g. binaryen and wasmedge interpret?
    auto _module = factory.loadCompiled(path);
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
    auto &module = _module.value();
    return module->instantiate().value();
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

  // maybe rely on "wabtValidate" used by "instrumentCodeForCompilation"
  test("binaryen", *kagome::runtime::binaryen::compiler);
  test("wasmedge-interpret", *kagome::runtime::wasm_edge::interpreter());
  test("wasmedge-compile", *kagome::runtime::wasm_edge::compiler());
  fmt::println("done");

  trie_storage.reset();
  kagome::runtime::binaryen::reset();
}
