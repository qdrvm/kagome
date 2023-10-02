/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_factory_impl.hpp"

#include <wasmedge/wasmedge.h>

#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wasm_edge/core_api_factory_impl.hpp"
#include "runtime/wasm_edge/register_host_api.hpp"

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

  template <typename T, void (*Deleter)(T)>
  class Wrapper {
   public:
    Wrapper() : t{} {}

    Wrapper(T t) : t{std::move(t)} {}

    Wrapper(const Wrapper &) = delete;
    Wrapper &operator=(const Wrapper &) = delete;

    Wrapper(Wrapper &&other) {
      if (t) {
        Deleter(t);
      }
      t = other.t;
      other.t = nullptr;
    }

    Wrapper &operator=(Wrapper &&other) {
      if (t) {
        Deleter(t);
      }
      t = other.t;
      other.t = nullptr;
      return *this;
    }

    ~Wrapper() {
      if constexpr (std::is_pointer_v<T>) {
        if (t) {
          Deleter(t);
        }
      }
    }

    T &raw() {
      return t;
    }

    const T &raw() const {
      return t;
    }

    T *operator->() {
      return t;
    }

    const T *operator->() const {
      return t;
    }

    const T *operator&() const {
      return &t;
    }

    T *operator&() {
      return &t;
    }

    bool operator==(std::nullptr_t) const {
      return t == nullptr;
    }

    bool operator!=(std::nullptr_t) const {
      return t != nullptr;
    }

    T t = nullptr;
  };

  using ConfigureContext =
      Wrapper<WasmEdge_ConfigureContext *, WasmEdge_ConfigureDelete>;
  using LoaderContext =
      Wrapper<WasmEdge_LoaderContext *, WasmEdge_LoaderDelete>;
  using StatsContext =
      Wrapper<WasmEdge_StatisticsContext *, WasmEdge_StatisticsDelete>;
  using FunctionTypeContext =
      Wrapper<WasmEdge_FunctionTypeContext *, WasmEdge_FunctionTypeDelete>;
  using FunctionInstanceContext = Wrapper<WasmEdge_FunctionInstanceContext *,
                                          WasmEdge_FunctionInstanceDelete>;
  using ExecutorContext =
      Wrapper<WasmEdge_ExecutorContext *, WasmEdge_ExecutorDelete>;
  using StoreContext = Wrapper<WasmEdge_StoreContext *, WasmEdge_StoreDelete>;
  using VmContext = Wrapper<WasmEdge_VMContext *, WasmEdge_VMDelete>;
  using ModuleInstanceContext =
      Wrapper<WasmEdge_ModuleInstanceContext *, WasmEdge_ModuleInstanceDelete>;
  using String = Wrapper<WasmEdge_String, WasmEdge_StringDelete>;
  using ASTModuleContext =
      Wrapper<WasmEdge_ASTModuleContext *, WasmEdge_ASTModuleDelete>;
  using MemoryInstanceContext =
      Wrapper<WasmEdge_MemoryInstanceContext *, WasmEdge_MemoryInstanceDelete>;
  using ValidatorContext =
      Wrapper<WasmEdge_ValidatorContext *, WasmEdge_ValidatorDelete>;

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
    explicit ModuleInstanceImpl(std::shared_ptr<const Module> module,
                                std::shared_ptr<ExecutorContext> executor,
                                ModuleInstanceContext instance_ctx,
                                InstanceEnvironment env,
                                const common::Hash256 &code_hash)
        : module_{module},
          instance_{std::move(instance_ctx)},
          executor_{executor},
          env_{std::move(env)},
          code_hash_{} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(instance_ != nullptr);
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
      ConfigureContext config_ctx = WasmEdge_ConfigureCreate();
      StoreContext store_ctx = WasmEdge_StoreCreate();

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
      return getEnvironment()
          .memory_provider->getCurrentMemory()
          .value()
          .get()
          .loadN(ptr, size);
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

    void forDataSegment(const DataSegmentProcessor &callback) const override {}

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
    std::shared_ptr<ExecutorContext> executor_;
    InstanceEnvironment env_;
    const common::Hash256 code_hash_;
  };

  class MemoryImpl final : public Memory {
   public:
    MemoryImpl(WasmEdge_MemoryInstanceContext *mem_instance,
               const MemoryConfig &config)
        : mem_instance_{std::move(mem_instance)},
          allocator_{MemoryAllocator{
              MemoryAllocator::MemoryHandle{
                  .resize = [this](size_t new_size) { resize(new_size); },
                  .getSize = [this]() -> size_t { return size(); },
                  .storeSz = [this](WasmPointer p,
                                    uint32_t n) { store32(p, n); },
                  .loadSz = [this](WasmPointer p) -> uint32_t {
                    return load32u(p);
                  }},
              config}} {
      BOOST_ASSERT(mem_instance_ != nullptr);
    }
    /**
     * @brief Return the size of the memory
     */
    WasmSize size() const override {
      return WasmEdge_MemoryInstanceGetPageSize(mem_instance_)
           * kMemoryPageSize;
    }

    /**
     * Resizes memory to the given size
     * @param new_size
     */
    void resize(WasmSize new_size) override {
      if (new_size > size()) {
        auto old_page_num = WasmEdge_MemoryInstanceGetPageSize(mem_instance_);
        auto new_page_num = (new_size + kMemoryPageSize - 1) / kMemoryPageSize;
        auto res = WasmEdge_MemoryInstanceGrowPage(mem_instance_,
                                                   new_page_num - old_page_num);
        BOOST_ASSERT(WasmEdge_ResultOK(res));
      }
    }

    WasmPointer allocate(WasmSize size) override {
      return allocator_.allocate(size);
    }

    std::optional<WasmSize> deallocate(WasmPointer ptr) override {
      return allocator_.deallocate(ptr);
    }

    template <typename T>
    T loadInt(WasmPointer addr) const {
      T data{};
      auto res = WasmEdge_MemoryInstanceGetData(
          mem_instance_, reinterpret_cast<uint8_t *>(&data), addr, sizeof(T));
      BOOST_ASSERT(WasmEdge_ResultOK(res));
      return data;
    }

    int8_t load8s(WasmPointer addr) const override {
      return loadInt<int8_t>(addr);
    }

    uint8_t load8u(WasmPointer addr) const override {
      return loadInt<uint8_t>(addr);
    }

    int16_t load16s(WasmPointer addr) const override {
      return loadInt<int16_t>(addr);
    }

    uint16_t load16u(WasmPointer addr) const override {
      return loadInt<uint16_t>(addr);
    }

    int32_t load32s(WasmPointer addr) const override {
      return loadInt<int32_t>(addr);
    }

    uint32_t load32u(WasmPointer addr) const override {
      return loadInt<uint32_t>(addr);
    }

    int64_t load64s(WasmPointer addr) const override {
      return loadInt<int64_t>(addr);
    }

    uint64_t load64u(WasmPointer addr) const override {
      return loadInt<uint64_t>(addr);
    }

    std::array<uint8_t, 16> load128(WasmPointer addr) const override {
      return loadInt<std::array<uint8_t, 16>>(addr);
    }

    common::BufferView loadN(WasmPointer addr, WasmSize n) const override {
      auto ptr = WasmEdge_MemoryInstanceGetPointer(mem_instance_, addr, n);
      BOOST_ASSERT(ptr);
      return common::BufferView{ptr, n};
    }

    std::string loadStr(WasmPointer addr, WasmSize n) const override {
      std::string res(n, ' ');
      auto span = loadN(addr, n);
      std::copy_n(span.begin(), n, res.begin());
      return res;
    }

    template <typename T>
    void storeInt(WasmPointer addr, T value) const {
      auto res = WasmEdge_MemoryInstanceSetData(
          mem_instance_, reinterpret_cast<uint8_t *>(&value), addr, sizeof(T));
      BOOST_ASSERT(WasmEdge_ResultOK(res));
    }

    void store8(WasmPointer addr, int8_t value) override {
      storeInt<int8_t>(addr, value);
    }

    void store16(WasmPointer addr, int16_t value) override {
      storeInt<int16_t>(addr, value);
    }

    void store32(WasmPointer addr, int32_t value) override {
      storeInt<int32_t>(addr, value);
    }

    void store64(WasmPointer addr, int64_t value) override {
      storeInt<int64_t>(addr, value);
    }

    void store128(WasmPointer addr,
                  const std::array<uint8_t, 16> &value) override {
      storeBuffer(addr, value);
    }

    void storeBuffer(WasmPointer addr,
                     gsl::span<const uint8_t> value) override {
      auto res = WasmEdge_MemoryInstanceSetData(
          mem_instance_, value.data(), addr, value.size());
      if (!WasmEdge_ResultOK(res)) {
        SL_ERROR(logger_, "{}", WasmEdge_ResultGetMessage(res));
        std::abort();
      }
    }

    /**
     * @brief allocates buffer in memory and copies value into memory
     * @param value buffer to store
     * @return full wasm pointer to allocated buffer
     */
    WasmSpan storeBuffer(gsl::span<const uint8_t> value) override {
      auto ptr = allocate(value.size());
      storeBuffer(ptr, value);
      return PtrSize{ptr, static_cast<WasmSize>(value.size())}.combine();
    }

   private:
    WasmEdge_MemoryInstanceContext *mem_instance_;
    MemoryAllocator allocator_;
    log::Logger logger_ = log::createLogger("Memory", "runtime");
  };

  class MemoryProviderImpl final : public MemoryProvider {
   public:
    explicit MemoryProviderImpl(WasmEdge_MemoryInstanceContext *wasmedge_memory)
        : wasmedge_memory_{wasmedge_memory} {
      BOOST_ASSERT(wasmedge_memory_);
    }
    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override {
      if (current_memory_) {
        return std::reference_wrapper<runtime::Memory>(**current_memory_);
      }
      return std::nullopt;
    }

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override {
      current_memory_ = std::make_shared<MemoryImpl>(wasmedge_memory_, config);
      return outcome::success();
    }

   private:
    std::optional<std::shared_ptr<MemoryImpl>> current_memory_;
    WasmEdge_MemoryInstanceContext *wasmedge_memory_;
  };

  class InstanceEnvironmentFactory {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<CoreApiFactory> core_factory,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<TrieStorageProvider> storage_provider)
        : core_factory_{core_factory},
          host_api_factory_{host_api_factory},
          storage_provider_{storage_provider} {}

    InstanceEnvironment make(WasmEdge_MemoryInstanceContext *memory) {
      auto memory_provider = std::make_shared<MemoryProviderImpl>(memory);

      return InstanceEnvironment{
          memory_provider,
          storage_provider_,
          host_api_factory_->make(
              core_factory_, memory_provider, storage_provider_),
          {}};
    }

   private:
    std::shared_ptr<CoreApiFactory> core_factory_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
  };

  class ModuleImpl : public Module,
                     public std::enable_shared_from_this<ModuleImpl> {
   public:
    static std::shared_ptr<ModuleImpl> create(
        ASTModuleContext module,
        std::shared_ptr<ExecutorContext> executor,
        StoreContext store,
        std::shared_ptr<InstanceEnvironmentFactory> env_factory,
        const common::Hash256 &code_hash) {
      return std::shared_ptr<ModuleImpl>{new ModuleImpl{std::move(module),
                                                        std::move(executor),
                                                        std::move(store),
                                                        std::move(env_factory),
                                                        code_hash}};
    }

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override {
      ModuleInstanceContext instance_ctx;

      auto *host_instance =
          WasmEdge_ModuleInstanceCreate(WasmEdge_StringCreateByCString("env"));

      uint32_t imports_num = WasmEdge_ASTModuleListImportsLength(module_.raw());
      std::vector<WasmEdge_ImportTypeContext *> imports;
      imports.resize(imports_num);
      WasmEdge_ASTModuleListImports(
          module_.raw(),
          const_cast<const WasmEdge_ImportTypeContext **>(imports.data()),
          imports_num);

      const WasmEdge_MemoryTypeContext *memory_type{};
      using namespace std::string_view_literals;
      static const auto memory_name = WasmEdge_StringCreateByCString("memory");
      for (auto &import : imports) {
        if (WasmEdge_StringIsEqual(
                memory_name, WasmEdge_ImportTypeGetExternalName(import))) {
          memory_type = WasmEdge_ImportTypeGetMemoryType(module_.raw(), import);
        }
      }
      BOOST_ASSERT(memory_type);
      WasmEdge_MemoryInstanceContext *mem_instance =
          WasmEdge_MemoryInstanceCreate(memory_type);
      InstanceEnvironment env = env_factory_->make(mem_instance);

      WasmEdge_ModuleInstanceAddMemory(
          host_instance, memory_name, mem_instance);

      register_host_api(*env.host_api, host_instance);

      WasmEdge_UNWRAP(WasmEdge_ExecutorRegisterImport(
          executor_->raw(), store_.raw(), host_instance));

      WasmEdge_UNWRAP(WasmEdge_ExecutorInstantiate(
          executor_->raw(), &instance_ctx.raw(), store_.raw(), module_.raw()));

      return std::make_shared<ModuleInstanceImpl>(shared_from_this(),
                                                  executor_,
                                                  std::move(instance_ctx),
                                                  std::move(env),
                                                  code_hash_);
    }

   private:
    explicit ModuleImpl(ASTModuleContext module,
                        std::shared_ptr<ExecutorContext> executor,
                        StoreContext store,
                        std::shared_ptr<InstanceEnvironmentFactory> env_factory,
                        common::Hash256 code_hash)
        : env_factory_{std::move(env_factory)},

          executor_{std::move(executor)},
          module_{std::move(module)},
          store_{std::move(store)},

          code_hash_{code_hash} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
      BOOST_ASSERT(env_factory_ != nullptr);
    }

    std::shared_ptr<InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<ExecutorContext> executor_;
    ASTModuleContext module_;
    StoreContext store_;
    const common::Hash256 code_hash_;
  };

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : hasher_{hasher},
        host_api_factory_{host_api_factory},
        storage_{storage},
        serializer_{serializer},
        header_repo_{header_repo} {
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(host_api_factory_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(serializer_);
    BOOST_ASSERT(header_repo_);
  }

  outcome::result<std::shared_ptr<Module>> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    ConfigureContext configure_ctx = WasmEdge_ConfigureCreate();
    if (configure_ctx == nullptr) {
      // return error
    }
    LoaderContext loader_ctx = WasmEdge_LoaderCreate(configure_ctx.raw());
    WasmEdge_ASTModuleContext *module_ctx;
    WasmEdge_UNWRAP(WasmEdge_LoaderParseFromBuffer(
        loader_ctx.raw(), &module_ctx, code.data(), code.size()));
    ASTModuleContext module = module_ctx;

    ValidatorContext validator = WasmEdge_ValidatorCreate(configure_ctx.raw());
    WasmEdge_UNWRAP(WasmEdge_ValidatorValidate(validator.raw(), module.raw()));
    auto executor = std::make_shared<ExecutorContext>(
        WasmEdge_ExecutorCreate(nullptr, nullptr));
    StoreContext store = WasmEdge_StoreCreate();

    auto core_api = std::make_shared<CoreApiFactoryImpl>(shared_from_this());
    auto storage_provider =
        std::make_shared<TrieStorageProviderImpl>(storage_, serializer_);

    auto env_factory = std::make_shared<InstanceEnvironmentFactory>(
        core_api, host_api_factory_, storage_provider);

    auto code_hash = hasher_->sha2_256(code);
    return ModuleImpl::create(std::move(module),
                              std::move(executor),
                              std::move(store),
                              env_factory,
                              code_hash);
  }

}  // namespace kagome::runtime::wasm_edge
