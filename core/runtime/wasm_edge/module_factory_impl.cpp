/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_factory_impl.hpp"

#include <filesystem>

#include <wasmedge/wasmedge.h>
#include <libp2p/common/final_action.hpp>

#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "log/trace_macros.hpp"
#include "runtime/common/core_api_factory_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
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

#define WasmEdge_UNWRAP_COMPILE_ERR(expr)                                      \
  if (auto _wasm_edge_res = (expr); !WasmEdge_ResultOK(_wasm_edge_res)) {      \
    return CompilationError(                                                   \
        fmt::format(#expr ": {}", WasmEdge_ResultGetMessage(_wasm_edge_res))); \
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
          code_hash_{code_hash} {
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
        args_ptrsize = PtrSize{getEnvironment()
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
      if (!func) {
        return RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND;
      }

      SL_TRACE(
          log_,
          "Invoke env for {}:\n"
          "\tmodule_instance: {}\n"
          "\tinternal module_instance: {}\n"
          "\thost module_instance: {}\n"
          "\thost_api: {}\n"
          "\tmemory_provider: {}\n"
          "\tmemory: {}\n"
          "\tstorage_provider: {}",
          name,
          fmt::ptr(this),
          fmt::ptr(instance_.raw()),
          fmt::ptr(host_instance_),
          fmt::ptr(env_.host_api),
          fmt::ptr(env_.memory_provider),
          fmt::ptr(&env_.memory_provider->getCurrentMemory().value().get()),
          fmt::ptr(env_.storage_provider));

      auto res = WasmEdge_ExecutorInvoke(executor_->raw(),
                                         func,
                                         params.data(),
                                         params.size(),
                                         returns.data(),
                                         1);
      WasmEdge_UNWRAP(res);
      WasmSpan span = WasmEdge_ValueGetI64(returns[0]);
      OUTCOME_TRY(
          view,
          getEnvironment().memory_provider->getCurrentMemory()->get().view(
              span));
      return common::Buffer{view};
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

    void forDataSegment(const DataSegmentProcessor &callback) const override;

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
    log::Logger log_ = log::createLogger("ModuleInstance", "runtime");
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
    explicit ModuleImpl(ASTModuleContext module,
                        std::shared_ptr<ExecutorContext> executor,
                        std::shared_ptr<InstanceEnvironmentFactory> env_factory,
                        const WasmEdge_MemoryTypeContext *memory_type,
                        common::Hash256 code_hash)
        : env_factory_{std::move(env_factory)},
          executor_{std::move(executor)},
          memory_type_{memory_type},
          module_{std::move(module)},
          code_hash_{code_hash} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
      BOOST_ASSERT(env_factory_ != nullptr);
    }

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override {
      StoreContext store = WasmEdge_StoreCreate();

      auto host_instance = std::make_shared<ModuleInstanceContext>(
          WasmEdge_ModuleInstanceCreate(WasmEdge_StringCreateByCString("env")));

      std::shared_ptr<MemoryProvider> memory_provider;
      if (memory_type_) {
        auto *mem_instance = WasmEdge_MemoryInstanceCreate(memory_type_);
        auto limit = WasmEdge_MemoryTypeGetLimit(memory_type_);
        SL_DEBUG(log_,
                 "Create memory instance, min: {}, max: {}",
                 limit.Min,
                 limit.Max);
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

      register_host_api(*env.host_api, module_.raw(), host_instance->raw());
      WasmEdge_UNWRAP(WasmEdge_ExecutorRegisterImport(
          executor_->raw(), store.raw(), host_instance->raw()));

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
    std::shared_ptr<InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<ExecutorContext> executor_;
    log::Logger log_ = log::createLogger("Module", "runtime");
    const WasmEdge_MemoryTypeContext *memory_type_;
    ASTModuleContext module_;
    const common::Hash256 code_hash_;

    friend class ModuleInstanceImpl;
  };

  void ModuleInstanceImpl::forDataSegment(
      const DataSegmentProcessor &callback) const {
    auto raw = dynamic_cast<const ModuleImpl &>(*module_).module_.raw();
    uint32_t segments_num = WasmEdge_ASTModuleListDataSegments(raw, nullptr, 0);
    std::vector<WasmEdge_DataSegment> segments(segments_num);
    WasmEdge_ASTModuleListDataSegments(raw, segments.data(), segments.size());
    for (auto &segment : segments) {
      callback(segment.Offset, std::span{segment.Data, segment.Length});
    }
  }

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      Config config)
      : hasher_{hasher},
        host_api_factory_{host_api_factory},
        storage_{storage},
        serializer_{serializer},
        log_{log::createLogger("ModuleFactory", "runtime")},
        config_{config} {
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(host_api_factory_);
  }

  outcome::result<std::shared_ptr<Module>, CompilationError>
  ModuleFactoryImpl::make(common::BufferView code) const {
    auto code_hash = hasher_->sha2_256(code);

    ConfigureContext configure_ctx = WasmEdge_ConfigureCreate();
    BOOST_ASSERT(configure_ctx.raw() != nullptr);  // no known reasons to fail

    LoaderContext loader_ctx = WasmEdge_LoaderCreate(configure_ctx.raw());
    WasmEdge_ASTModuleContext *module_ctx;

    switch (config_.exec) {
      case ExecType::Compiled: {
        WasmEdge_ConfigureCompilerSetOptimizationLevel(
            configure_ctx.raw(), WasmEdge_CompilerOptimizationLevel_O3);
        CompilerContext compiler = WasmEdge_CompilerCreate(configure_ctx.raw());
        std::string filename = fmt::format("{}/wasm_{}",
                                           config_.compiled_module_dir.c_str(),
                                           code_hash.toHex());
        std::error_code ec;
        if (!std::filesystem::create_directories(config_.compiled_module_dir,
                                                 ec)
            && ec) {
          return CompilationError{fmt::format(
              "Failed to create a dir for compiled modules: {}", ec.message())};
        }
        if (!std::filesystem::exists(filename)) {
          SL_INFO(log_, "Start compiling wasm module {}…", code_hash);
          WasmEdge_UNWRAP_COMPILE_ERR(WasmEdge_CompilerCompileFromBuffer(
              compiler.raw(), code.data(), code.size(), filename.c_str()));
          SL_INFO(log_, "Compilation finished, saved at {}", filename);
        }
        WasmEdge_UNWRAP_COMPILE_ERR(WasmEdge_LoaderParseFromFile(
            loader_ctx.raw(), &module_ctx, filename.c_str()));
        break;
      }
      case ExecType::Interpreted: {
        WasmEdge_UNWRAP_COMPILE_ERR(WasmEdge_LoaderParseFromBuffer(
            loader_ctx.raw(), &module_ctx, code.data(), code.size()));
        break;
      }
      default:
        BOOST_UNREACHABLE_RETURN({});
    }
    ASTModuleContext module = module_ctx;

    ValidatorContext validator = WasmEdge_ValidatorCreate(configure_ctx.raw());
    WasmEdge_UNWRAP_COMPILE_ERR(
        WasmEdge_ValidatorValidate(validator.raw(), module.raw()));

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

    return std::shared_ptr<ModuleImpl>{new ModuleImpl{std::move(module),
                                                      std::move(executor),
                                                      env_factory,
                                                      import_memory_type,
                                                      code_hash}};
  }

}  // namespace kagome::runtime::wasm_edge
