/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_factory_impl.hpp"

#include <filesystem>

#include <wasmedge/wasmedge.h>

#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "log/trace_macros.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wasm_edge/core_api_factory_impl.hpp"
#include "runtime/wasm_edge/memory_impl.hpp"
#include "runtime/wasm_edge/register_host_api.hpp"
#include "runtime/wasm_edge/wrappers.hpp"

namespace kagome::runtime::wasm_edge {
  enum class Error {
    INVALID_VALUE_TYPE = 1,

  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::wasm_edge, Error);

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::wasm_edge, Error, e) {
  using E = kagome::runtime::wasm_edge::Error;
  switch (e) {
    case E::INVALID_VALUE_TYPE:
      return "Invalid value type";
  }
  return "Unknown WasmEdge error";
}

namespace kagome::runtime::wasm_edge {

  // stack of HostApis currently in use by runtime calls
  // (stack because there are nested runtime calls like Core_version)
  thread_local std::stack<std::shared_ptr<host_api::HostApi>> current_host_api;

  static const auto kMemoryName = WasmEdge_StringCreateByCString("memory");

  class WasmEdgeErrCategory final : public std::error_category {
   public:
    const char *name() const noexcept override {
      return "WasmEdge";
    }

    std::string message(int code) const override {
      auto res = WasmEdge_ResultGen(WasmEdge_ErrCategory_WASM, code);
      return WasmEdge_ResultGetMessage(res);
    }
  };

  WasmEdgeErrCategory wasm_edge_err_category;

  std::error_code make_error_code(WasmEdge_Result res) {
    BOOST_ASSERT(WasmEdge_ResultGetCategory(res) == WasmEdge_ErrCategory_WASM);
    return std::error_code{static_cast<int>(WasmEdge_ResultGetCode(res)),
                           wasm_edge_err_category};
  }

#define WasmEdge_UNWRAP(expr)                                             \
  if (auto _wasm_edge_res = (expr); !WasmEdge_ResultOK(_wasm_edge_res)) { \
    return make_error_code(_wasm_edge_res);                               \
  }

  static outcome::result<WasmValue> convertValue(WasmEdge_Value v) {
    switch (v.Type) {
      case WasmEdge_ValType_I32:
        return WasmEdge_ValueGetI32(v);
      case WasmEdge_ValType_I64:
        return WasmEdge_ValueGetI64(v);
      case WasmEdge_ValType_F32:
        return WasmEdge_ValueGetF32(v);
      case WasmEdge_ValType_F64:
        return WasmEdge_ValueGetF64(v);
      case WasmEdge_ValType_V128:
        return Error::INVALID_VALUE_TYPE;
      case WasmEdge_ValType_FuncRef:
        return Error::INVALID_VALUE_TYPE;
      case WasmEdge_ValType_ExternRef:
        return Error::INVALID_VALUE_TYPE;
    }
    BOOST_UNREACHABLE_RETURN({});
  }

  class ModuleInstanceImpl : public ModuleInstance {
   public:
    explicit ModuleInstanceImpl(
        std::shared_ptr<const Module> module,
        std::shared_ptr<ExecutorContext> executor,
        ModuleInstanceContext instance_ctx,
        std::shared_ptr<ModuleInstanceContext> host_instance,
        InstanceEnvironment env,
        const common::Hash256 &code_hash)
        : module_{module},
          instance_{std::move(instance_ctx)},
          host_instance_{host_instance},
          executor_{executor},
          env_{std::move(env)},
          code_hash_{} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(instance_ != nullptr);
      BOOST_ASSERT(host_instance_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
    }

    const common::Hash256 &getCodeHash() const override {
      return code_hash_;
    }

    std::shared_ptr<const Module> getModule() const override {
      return module_;
    }

    outcome::result<common::Buffer> callExportFunction(
        RuntimeContext &ctx,
        std::string_view name,
        common::BufferView encoded_args) const override {
      PtrSize args_ptrsize{};
      if (!encoded_args.empty()) {
        args_ptrsize = PtrSize{ctx.module_instance->getEnvironment()
                                   .memory_provider->getCurrentMemory()
                                   .value()
                                   .get()
                                   .storeBuffer(encoded_args)};
      }
      std::array params{WasmEdge_ValueGenI32(args_ptrsize.ptr),
                        WasmEdge_ValueGenI32(args_ptrsize.size)};
      std::array returns{WasmEdge_ValueGenI64(0)};
      String wasm_name =
          WasmEdge_StringCreateByBuffer(name.data(), name.size());
      auto func =
          WasmEdge_ModuleInstanceFindFunction(instance_.raw(), wasm_name.raw());

      current_host_api.push(env_.host_api);
      auto res = WasmEdge_ExecutorInvoke(executor_->raw(),
                                         func,
                                         params.data(),
                                         params.size(),
                                         returns.data(),
                                         1);
      if (!WasmEdge_ResultOK(res)) {
        return make_error_code(res);
      }
      auto [ptr, size] = PtrSize{WasmEdge_ValueGetI64(returns[0])};
      auto result = getEnvironment()
                        .memory_provider->getCurrentMemory()
                        .value()
                        .get()
                        .loadN(ptr, size);
      BOOST_ASSERT(!current_host_api.empty());
      BOOST_ASSERT(current_host_api.top() == env_.host_api);
      current_host_api.pop();
      return result;
    }

    outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name_) const override {
      String name = WasmEdge_StringCreateByBuffer(name_.data(), name_.size());
      auto global =
          WasmEdge_ModuleInstanceFindGlobal(instance_.raw(), name.raw());
      if (global == nullptr) {
        return std::nullopt;
      }
      auto value = WasmEdge_GlobalInstanceGetValue(global);
      OUTCOME_TRY(v, convertValue(value));
      return v;
    }

    void forDataSegment(const DataSegmentProcessor &callback) const override {
      uint32_t segments_num =
          WasmEdge_ModuleInstanceListDataSegments(instance_.raw(), nullptr, 0);
      std::vector<WasmEdge_DataSegment> segments(segments_num);
      WasmEdge_ModuleInstanceListDataSegments(
          instance_.raw(), segments.data(), segments.size());
      for (auto &segment : segments) {
        callback(segment.Offset, std::span{segment.Data, segment.Length});
      }
    }

    const InstanceEnvironment &getEnvironment() const override {
      return env_;
    }

    outcome::result<void> resetEnvironment() override {
      env_.host_api->reset();
      return outcome::success();
    }

   private:
    std::shared_ptr<const Module> module_;
    ModuleInstanceContext instance_;
    std::shared_ptr<ModuleInstanceContext> host_instance_;
    std::shared_ptr<ExecutorContext> executor_;
    InstanceEnvironment env_;
    const common::Hash256 code_hash_;
  };

  class InstanceEnvironmentFactory {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<CoreApiFactory> core_factory,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer)
        : core_factory_{core_factory},
          host_api_factory_{host_api_factory},
          storage_{storage},
          serializer_{serializer} {}

    InstanceEnvironment make(std::shared_ptr<MemoryProvider> memory_provider) {
      auto storage_provider =
          std::make_shared<TrieStorageProviderImpl>(storage_, serializer_);

      return InstanceEnvironment{
          memory_provider,
          storage_provider,
          host_api_factory_->make(
              core_factory_, memory_provider, storage_provider),
          {}};
    }

   private:
    std::shared_ptr<CoreApiFactory> core_factory_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
  };

  class ModuleImpl : public Module,
                     public std::enable_shared_from_this<ModuleImpl> {
   public:
    static std::shared_ptr<ModuleImpl> create(
        ASTModuleContext module,
        std::shared_ptr<ExecutorContext> executor,
        std::shared_ptr<InstanceEnvironmentFactory> env_factory,
        // std::shared_ptr<ModuleInstanceContext> host_instance,
        const WasmEdge_MemoryTypeContext *memory_type,
        const common::Hash256 &code_hash) {
      return std::shared_ptr<ModuleImpl>{new ModuleImpl{std::move(module),
                                                        std::move(executor),
                                                        std::move(env_factory),
                                                        // host_instance,
                                                        memory_type,
                                                        code_hash}};
    }

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override {
      StoreContext store = WasmEdge_StoreCreate();
      auto host_instance = std::make_shared<ModuleInstanceContext>(
          WasmEdge_ModuleInstanceCreate(WasmEdge_StringCreateByCString("env")));
      register_host_api(host_instance->raw());
      WasmEdge_UNWRAP(WasmEdge_ExecutorRegisterImport(
          executor_->raw(), store.raw(), host_instance->raw()));

      std::shared_ptr<MemoryProvider> memory_provider;
      if (memory_type_) {
        auto *mem_instance = WasmEdge_MemoryInstanceCreate(memory_type_);
        WasmEdge_ModuleInstanceAddMemory(
            host_instance->raw(), kMemoryName, mem_instance);

        mem_instance = WasmEdge_ModuleInstanceFindMemory(host_instance->raw(),
                                                         kMemoryName);
        memory_provider =
            std::make_shared<ExternalMemoryProviderImpl>(mem_instance);
      } else {
        memory_provider = std::make_shared<InternalMemoryProviderImpl>();
      }

      InstanceEnvironment env = env_factory_->make(memory_provider);

      ModuleInstanceContext instance_ctx;
      WasmEdge_UNWRAP(WasmEdge_ExecutorInstantiate(
          executor_->raw(), &instance_ctx.raw(), store.raw(), module_.raw()));

      if (!memory_type_) {
        auto memory_ctx =
            WasmEdge_ModuleInstanceFindMemory(instance_ctx.raw(), kMemoryName);
        BOOST_ASSERT(memory_ctx);
        static_cast<InternalMemoryProviderImpl *>(memory_provider.get())
            ->setMemory(memory_ctx);
      }

      return std::make_shared<ModuleInstanceImpl>(shared_from_this(),
                                                  executor_,
                                                  std::move(instance_ctx),
                                                  host_instance,
                                                  std::move(env),
                                                  code_hash_);
    }

   private:
    explicit ModuleImpl(ASTModuleContext module,
                        std::shared_ptr<ExecutorContext> executor,
                        std::shared_ptr<InstanceEnvironmentFactory> env_factory,
                        // std::shared_ptr<ModuleInstanceContext> host_instance,
                        const WasmEdge_MemoryTypeContext *memory_type,
                        common::Hash256 code_hash)
        : env_factory_{std::move(env_factory)},
          executor_{std::move(executor)},
          // host_instance_{host_instance},
          memory_type_{memory_type},
          module_{std::move(module)},
          code_hash_{code_hash} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
      BOOST_ASSERT(env_factory_ != nullptr);
      // BOOST_ASSERT(host_instance_ != nullptr);
    }

    std::shared_ptr<InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<ExecutorContext> executor_;
    // std::shared_ptr<ModuleInstanceContext> host_instance_;
    const WasmEdge_MemoryTypeContext *memory_type_;
    ASTModuleContext module_;
    const common::Hash256 code_hash_;
  };

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      Config config)
      : hasher_{hasher},
        host_api_factory_{host_api_factory},
        storage_{storage},
        serializer_{serializer},
        header_repo_{header_repo},
        log_{log::createLogger("ModuleFactory", "runtime")},
        config_{config} {
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(host_api_factory_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(serializer_);
    BOOST_ASSERT(header_repo_);
  }

  outcome::result<std::shared_ptr<Module>> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    auto code_hash = hasher_->sha2_256(code);

    ConfigureContext configure_ctx = WasmEdge_ConfigureCreate();
    if (configure_ctx == nullptr) {
      // return error
    }

    LoaderContext loader_ctx = WasmEdge_LoaderCreate(configure_ctx.raw());
    WasmEdge_ASTModuleContext *module_ctx;

    switch (config_.exec) {
      case ExecType::Compiled: {
        CompilerContext compiler = WasmEdge_CompilerCreate(configure_ctx.raw());
        std::string dirname = "/tmp/kagome";
        std::string filename =
            fmt::format("{}/wasm_{}", dirname, code_hash.toHex());
        std::error_code ec;
        if (!std::filesystem::create_directories(dirname, ec) && ec) {
          return ec;
        }
        SL_INFO(log_, "Start compiling wasm module {}...", code_hash);
        WasmEdge_UNWRAP(WasmEdge_CompilerCompileFromBuffer(
            compiler.raw(), code.data(), code.size(), filename.c_str()));
        SL_INFO(log_, "Compilation finished");
        WasmEdge_UNWRAP(WasmEdge_LoaderParseFromFile(
            loader_ctx.raw(), &module_ctx, filename.c_str()));
        break;
      }
      case ExecType::Interpreted: {
        WasmEdge_UNWRAP(WasmEdge_LoaderParseFromBuffer(
            loader_ctx.raw(), &module_ctx, code.data(), code.size()));
        break;
      }
      default:
        BOOST_UNREACHABLE_RETURN({});
    }
    ASTModuleContext module = module_ctx;

    ValidatorContext validator = WasmEdge_ValidatorCreate(configure_ctx.raw());
    WasmEdge_UNWRAP(WasmEdge_ValidatorValidate(validator.raw(), module.raw()));

    auto executor = std::make_shared<ExecutorContext>(
        WasmEdge_ExecutorCreate(nullptr, nullptr));

    uint32_t imports_num = WasmEdge_ASTModuleListImportsLength(module.raw());
    std::vector<WasmEdge_ImportTypeContext *> imports;
    imports.resize(imports_num);
    WasmEdge_ASTModuleListImports(
        module.raw(),
        const_cast<const WasmEdge_ImportTypeContext **>(imports.data()),
        imports_num);

    const WasmEdge_MemoryTypeContext *import_memory_type{};
    using namespace std::string_view_literals;
    for (auto &import : imports) {
      if (WasmEdge_StringIsEqual(kMemoryName,
                                 WasmEdge_ImportTypeGetExternalName(import))) {
        import_memory_type =
            WasmEdge_ImportTypeGetMemoryType(module.raw(), import);
        break;
      }
    }

    auto core_api = std::make_shared<CoreApiFactoryImpl>(shared_from_this());

    auto env_factory = std::make_shared<InstanceEnvironmentFactory>(
        core_api, host_api_factory_, storage_, serializer_);

    return ModuleImpl::create(std::move(module),
                              std::move(executor),
                              env_factory,
                              // host_instance,
                              import_memory_type,
                              code_hash);
  }

}  // namespace kagome::runtime::wasm_edge
