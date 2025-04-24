/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_factory_impl.hpp"

#include <filesystem>

#include <wasmedge/wasmedge.h>
#include <libp2p/common/final_action.hpp>

#include "application/app_configuration.hpp"
#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "log/formatters/filepath.hpp"
#include "log/trace_macros.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wasm_compiler_definitions.hpp"  // this header-file is generated
#include "runtime/wasm_edge/memory_impl.hpp"
#include "runtime/wasm_edge/register_host_api.hpp"
#include "runtime/wasm_edge/wrappers.hpp"
#include "utils/read_file.hpp"
#include "utils/write_file.hpp"

static_assert(std::string_view{WASMEDGE_ID}.size() == 40,
              "WASMEDGE_ID should be set to WasmEdge repository SHA1 hash");

namespace kagome::runtime::wasm_edge {
  enum class Error : uint8_t {
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

  static const class final : public std::error_category {
   public:
    const char *name() const noexcept override {
      return "WasmEdge";
    }

    std::string message(int code) const override {
      auto res = WasmEdge_ResultGen(WasmEdge_ErrCategory_WASM, code);
      return WasmEdge_ResultGetMessage(res);
    }
  } wasm_edge_err_category;

  std::error_code make_error_code(WasmEdge_Result res) {
    BOOST_ASSERT(WasmEdge_ResultGetCategory(res) == WasmEdge_ErrCategory_WASM);
    return std::error_code{static_cast<int>(WasmEdge_ResultGetCode(res)),
                           wasm_edge_err_category};
  }

  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
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
    if (WasmEdge_ValTypeIsI32(v.Type)) {
      return WasmEdge_ValueGetI32(v);
    }
    if (WasmEdge_ValTypeIsI64(v.Type)) {
      return WasmEdge_ValueGetI64(v);
    }
    if (WasmEdge_ValTypeIsF32(v.Type)) {
      return WasmEdge_ValueGetF32(v);
    }
    if (WasmEdge_ValTypeIsF64(v.Type)) {
      return WasmEdge_ValueGetF64(v);
    }

    return Error::INVALID_VALUE_TYPE;
  }

  inline CompilationOutcome<ConfigureContext> configureCtx(
      const RuntimeContext::ContextParams &config) {
    ConfigureContext ctx{WasmEdge_ConfigureCreate()};
    auto ctx_raw = ctx.raw();
    if (ctx_raw == nullptr) {
      return CompilationError{"WasmEdge_ConfigureCreate returned nullptr"};
    }
    // by default WASM bulk memory operations are enabled
    if (not config.wasm_ext_bulk_memory) {
      WasmEdge_ConfigureRemoveProposal(ctx_raw,
                                       WasmEdge_Proposal_BulkMemoryOperations);
    }
    WasmEdge_ConfigureRemoveProposal(ctx_raw, WasmEdge_Proposal_ReferenceTypes);
    return ctx;
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
        : module_{std::move(module)},
          instance_{std::move(instance_ctx)},
          host_instance_{std::move(host_instance)},
          executor_{std::move(executor)},
          env_{std::move(env)},
          code_hash_{code_hash} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(instance_ != nullptr);
      BOOST_ASSERT(host_instance_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
    }

    common::Hash256 getCodeHash() const override {
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
      std::array params{
          WasmEdge_ValueGenI32(static_cast<int32_t>(args_ptrsize.ptr)),
          WasmEdge_ValueGenI32(static_cast<int32_t>(args_ptrsize.size))};
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

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const common::Hash256 code_hash_;
  };

  class InstanceEnvironmentFactory {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<CoreApiFactory> core_factory,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer)
        : core_factory_{std::move(core_factory)},
          host_api_factory_{std::move(host_api_factory)},
          storage_{std::move(storage)},
          serializer_{std::move(serializer)} {}

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

      auto env_name = WasmEdge_StringCreateByCString("env");

      auto host_instance = std::make_shared<ModuleInstanceContext>(
          WasmEdge_ModuleInstanceCreate(env_name));

      WasmEdge_StringDelete(env_name);

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

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
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

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
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
      std::shared_ptr<CoreApiFactory> core_factory,
      Config config)
      : hasher_{std::move(hasher)},
        host_api_factory_{std::move(host_api_factory)},
        storage_{std::move(storage)},
        serializer_{std::move(serializer)},
        core_factory_{std::move(core_factory)},
        log_{log::createLogger("ModuleFactory", "runtime")},
        config_{config} {
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(host_api_factory_);
  }

  std::optional<std::string_view> ModuleFactoryImpl::compilerType() const {
    if (config_.exec == ExecType::Interpreted) {
      return std::nullopt;
    }
    // version changes rarely, don't need the whole hash
    static std::string versioned_str =
        fmt::format("wasmedge_{}", std::string_view{WASMEDGE_ID}.substr(0, 12));
    return versioned_str;
  }

  CompilationOutcome<void> ModuleFactoryImpl::compile(
      std::filesystem::path path_compiled,
      BufferView code,
      const RuntimeContext::ContextParams &config) const {
    if (config_.exec == ExecType::Interpreted) {
      OUTCOME_TRY(writeFileTmp(path_compiled, code));
      return outcome::success();
    }

    OUTCOME_TRY(configure_ctx, configureCtx(config));
    auto configure_ctx_raw = configure_ctx.raw();
    WasmEdge_ConfigureCompilerSetOptimizationLevel(
        configure_ctx_raw, WasmEdge_CompilerOptimizationLevel_O2);

    CompilerContext compiler = WasmEdge_CompilerCreate(configure_ctx_raw);
    SL_INFO(log_, "Start compiling wasm module {}", path_compiled);
    // Multiple processes can write to same cache file concurrently,
    // write to tmp file first to avoid conflict.
    OUTCOME_TRY(tmp, TmpFile::make(path_compiled));
    WasmEdge_UNWRAP_COMPILE_ERR(WasmEdge_CompilerCompileFromBuffer(
        compiler.raw(), code.data(), code.size(), tmp.path().c_str()));
    OUTCOME_TRY(tmp.rename());
    SL_INFO(log_, "Compilation finished, saved at {}", path_compiled);
    return outcome::success();
  }

  CompilationOutcome<std::shared_ptr<Module>> ModuleFactoryImpl::loadCompiled(
      std::filesystem::path path_compiled,
      const RuntimeContext::ContextParams &config) const {
    Buffer code;
    if (auto res = readFile(code, path_compiled); !res) {
      return CompilationError{
          fmt::format("Failed to read compiled wasm module from '{}': {}",
                      path_compiled,
                      res.error())};
    }
    auto code_hash = hasher_->blake2b_256(code);
    OUTCOME_TRY(configure_ctx, configureCtx(config));
    auto configure_ctx_raw = configure_ctx.raw();
    LoaderContext loader_ctx = WasmEdge_LoaderCreate(configure_ctx_raw);
    // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
    WasmEdge_ASTModuleContext *module_ctx;
    WasmEdge_UNWRAP_COMPILE_ERR(WasmEdge_LoaderParseFromFile(
        loader_ctx.raw(), &module_ctx, path_compiled.c_str()));
    ASTModuleContext module = module_ctx;

    ValidatorContext validator = WasmEdge_ValidatorCreate(configure_ctx_raw);
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

    auto env_factory = std::make_shared<InstanceEnvironmentFactory>(
        core_factory_, host_api_factory_, storage_, serializer_);

    return std::make_shared<ModuleImpl>(std::move(module),
                                        std::move(executor),
                                        std::move(env_factory),
                                        import_memory_type,
                                        code_hash);
  }

}  // namespace kagome::runtime::wasm_edge
