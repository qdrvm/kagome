/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_factory_impl.hpp"

#include <wasmedge/wasmedge.h>

#include "host_api/host_api.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"

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

namespace kagome::runtime::wasm_edge {

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
  }

  class ModuleInstanceImpl : public ModuleInstance {
   public:
    explicit ModuleInstanceImpl(std::shared_ptr<const Module> module,
                                std::shared_ptr<ExecutorContext> executor,
                                ModuleInstanceContext instance_ctx,
                                InstanceEnvironment env,
                                const common::Hash256 &code_hash)
        : module_{module},
          executor_{executor},
          instance_{std::move(instance_ctx)},
          env_{std::move(env)},
          code_hash_{} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
      BOOST_ASSERT(instance_ != nullptr);
    }

    const common::Hash256 &getCodeHash() const override {
      return code_hash_;
    }

    std::shared_ptr<const Module> getModule() const override {
      return module_;
    }

    outcome::result<PtrSize> callExportFunction(
        std::string_view name, common::BufferView encoded_args) const override {
      ConfigureContext config_ctx = WasmEdge_ConfigureCreate();
      StoreContext store_ctx = WasmEdge_StoreCreate();
      VmContext vm_ctx = WasmEdge_VMCreate(config_ctx.raw(), store_ctx.raw());

      std::array params{WasmEdge_ValueGenI32(42), WasmEdge_ValueGenI32(42)};
      std::array returns{WasmEdge_ValueGenI64(42)};
      String wasm_name =
          WasmEdge_StringCreateByBuffer(name.data(), name.size());
      auto res = WasmEdge_VMExecute(vm_ctx.raw(),
                                    wasm_name.raw(),
                                    params.data(),
                                    params.size(),
                                    returns.data(),
                                    returns.size());
      if (!WasmEdge_ResultOK(res)) {
        return std::error_code{static_cast<int>(WasmEdge_ResultGetCode(res)),
                               WasmEdgeErrCategory{}};
      }
      return PtrSize{static_cast<uint64_t>(WasmEdge_ValueGetI64(returns[0]))};
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
#warning "Implement before PR"
    }

    const InstanceEnvironment &getEnvironment() const override {
      return env_;
    }

    outcome::result<void> resetEnvironment() override {
      env_.host_api->reset();
      return outcome::success();
    }

    outcome::result<void> resetMemory(const MemoryLimits &config) override {
#warning "Implement before PR"
      return outcome::success();
    }

   private:
    std::shared_ptr<const Module> module_;
    std::shared_ptr<ExecutorContext> executor_;
    ModuleInstanceContext instance_;
    InstanceEnvironment env_;
    const common::Hash256 code_hash_;
  };

  class MemoryImpl final : public Memory {
   public:
    MemoryImpl(MemoryInstanceContext mem_instance, const MemoryConfig &config)
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
      return WasmEdge_MemoryInstanceGetPageSize(mem_instance_.raw())
           * kMemoryPageSize;
    }

    /**
     * Resizes memory to the given size
     * @param new_size
     */
    void resize(WasmSize new_size) override {
      if (new_size > size()) {
        auto old_page_num =
            WasmEdge_MemoryInstanceGetPageSize(mem_instance_.raw());
        auto new_page_num = (new_size + kMemoryPageSize - 1) / kMemoryPageSize;
        auto res = WasmEdge_MemoryInstanceGrowPage(mem_instance_.raw(),
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
      auto res =
          WasmEdge_MemoryInstanceGetData(mem_instance_.raw(),
                                         reinterpret_cast<uint8_t *>(&data),
                                         addr,
                                         sizeof(T));
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
      auto ptr =
          WasmEdge_MemoryInstanceGetPointer(mem_instance_.raw(), addr, n);
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
      auto res =
          WasmEdge_MemoryInstanceSetData(mem_instance_.raw(),
                                         reinterpret_cast<uint8_t *>(&value),
                                         addr,
                                         sizeof(T));
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
      storeInt<std::array<uint8_t, 16>>(addr, value);
    }

    void storeBuffer(WasmPointer addr,
                     gsl::span<const uint8_t> value) override {
      auto res = WasmEdge_MemoryInstanceSetData(
          mem_instance_.raw(), value.data(), addr, value.size());
      BOOST_ASSERT(WasmEdge_ResultOK(res));
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
    MemoryInstanceContext mem_instance_;
    MemoryAllocator allocator_;
  };

  class MemoryProviderImpl final : public MemoryProvider {
   public:
    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override {
      if (current_memory_) {
        return std::reference_wrapper<runtime::Memory>(**current_memory_);
      }
      return std::nullopt;
    }

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override {
      MemoryInstanceContext mem_instance =
          WasmEdge_MemoryInstanceCreate(mem_type_);
      current_memory_ =
          std::make_shared<MemoryImpl>(std::move(mem_instance), config);
      return outcome::success();
    }

   private:
    std::optional<std::shared_ptr<MemoryImpl>> current_memory_;
    WasmEdge_MemoryTypeContext *mem_type_;
  };

  class ModuleImpl : public Module,
                     public std::enable_shared_from_this<ModuleImpl> {
   public:
    static std::shared_ptr<ModuleImpl> create(
        ASTModuleContext module,
        std::shared_ptr<ExecutorContext> executor,
        const common::Hash256 &code_hash) {
      return std::shared_ptr<ModuleImpl>{
          new ModuleImpl{std::move(module), executor, code_hash}};
    }

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override {
      StoreContext store_ctx = WasmEdge_StoreCreate();

      ModuleInstanceContext instance_ctx;
      auto res = WasmEdge_ExecutorInstantiate(executor_->raw(),
                                              &instance_ctx.raw(),
                                              store_ctx.raw(),
                                              module_.raw());
      if (!WasmEdge_ResultOK(res)) {
        BOOST_ASSERT(WasmEdge_ResultGetCategory(res)
                     == WasmEdge_ErrCategory_WASM);
        return std::error_code(WasmEdge_ResultGetCode(res),
                               WasmEdgeErrCategory{});
      }
      InstanceEnvironment instance_env{
          std::make_shared<MemoryProviderImpl>(),
          storage_provider_,
          host_api_,
          {},
      };
      return std::make_shared<ModuleInstanceImpl>(shared_from_this(),
                                                  executor_,
                                                  std::move(instance_ctx),
                                                  std::move(instance_env),
                                                  code_hash_);
    }

   private:
    explicit ModuleImpl(ASTModuleContext module,
                        std::shared_ptr<ExecutorContext> executor,
                        common::Hash256 code_hash)
        : module_{std::move(module)},
          executor_{executor},
          code_hash_{code_hash} {
      BOOST_ASSERT(module_ != nullptr);
      BOOST_ASSERT(executor_ != nullptr);
    }

    ASTModuleContext module_;
    std::shared_ptr<ExecutorContext> executor_;
    std::shared_ptr<host_api::HostApi> host_api_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    const common::Hash256 code_hash_;
  };

  outcome::result<std::shared_ptr<Module>> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    ConfigureContext configure_ctx = WasmEdge_ConfigureCreate();
    if (configure_ctx == nullptr) {
      // return error
    }
    LoaderContext loader_ctx = WasmEdge_LoaderCreate(configure_ctx.raw());
    WasmEdge_ASTModuleContext *module_ctx;
    auto res = WasmEdge_LoaderParseFromBuffer(
        loader_ctx.raw(), &module_ctx, code.data(), code.size());
    ASTModuleContext module = module_ctx;
    if (!WasmEdge_ResultOK(res)) {
      // return error
    }

    auto executor = std::make_shared<ExecutorContext>(
        WasmEdge_ExecutorCreate(nullptr, nullptr));

    auto code_hash = hasher_->sha2_256(code);
    return ModuleImpl::create(std::move(module), executor, code_hash);
  }

}  // namespace kagome::runtime::wasm_edge