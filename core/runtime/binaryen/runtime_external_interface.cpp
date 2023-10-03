/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_external_interface.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/memory.hpp"

namespace {
  /**
   * @brief one-to-one match of literal method to
   * C++ type
   */
  template <typename T>
  auto literalMemFun() {
    return nullptr;
  }

  template <>
  auto literalMemFun<int32_t>() {
    return &wasm::Literal::geti32;
  }

  template <>
  auto literalMemFun<int64_t>() {
    return &wasm::Literal::geti64;
  }

  /**
   * @brief a meta-layer that places list of arguments into host api method
   * invoсation using fold expression
   */
  template <typename T, typename R, auto mf, typename... Args, size_t... I>
  wasm::Literal callInternal(T *host_api,
                             const wasm::LiteralList &arguments,
                             std::index_sequence<I...>) {
    if constexpr (not std::is_same_v<void, R>) {
      // invokes literalMemFun method for every literal in a list to convert
      // wasm value into C++ value and pass it as argument
      return wasm::Literal(
          (host_api->*mf)((arguments.at(I).*literalMemFun<Args>())()...));
    } else {
      (host_api->*mf)((arguments.at(I).*literalMemFun<Args>())()...);
      return wasm::Literal();
    }
  }

  /**
   * @brief general HostApi method wrapper
   * Gets method type and value as template arguments. Has two separate
   * specializations for const and non-const HostApi methods
   */
  template <typename T, T>
  struct HostApiFunc;
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...)>
  struct HostApiFunc<R (T::*)(Args...), mf> {
    using Ret = R;
    static const size_t size = sizeof...(Args);
    static wasm::Literal call(T *host_api, const wasm::LiteralList &arguments) {
      // set index for every argument
      auto indices = std::index_sequence_for<Args...>{};
      return callInternal<T, R, mf, Args...>(host_api, arguments, indices);
    }
  };
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...) const>
  struct HostApiFunc<R (T::*)(Args...) const, mf> {
    using Ret = R;
    static const size_t size = sizeof...(Args);
    static wasm::Literal call(T *host_api, const wasm::LiteralList &arguments) {
      // set index for every argument
      auto indices = std::index_sequence_for<Args...>{};
      return callInternal<T, R, mf, Args...>(host_api, arguments, indices);
    }
  };

  /**
   * @brief gets number of arguments of method
   * @tparam mf method ref to invoke
   * @return argument list size
   */
  template <auto mf>
  constexpr size_t hostApiFuncArgSize() {
    return HostApiFunc<decltype(mf), mf>::size;
  }

  /**
   * @brief invokes host api method passed as a template argument
   * @tparam mf method ref to invoke
   * @param host_api HostApi implementation pointer
   * @param arguments list of arguments to invoke method with
   * @return implementation dependent anytype result
   */
  template <auto mf>
  wasm::Literal callHostApiFunc(kagome::host_api::HostApi *host_api,
                                const wasm::LiteralList &arguments) {
    std::cout << "Call Host method " << __PRETTY_FUNCTION__  << "\n";
    return HostApiFunc<decltype(mf), mf>::call(host_api, arguments);
  }
}  // namespace

/**
 * @brief check arg num and call HostApi method macro
 * Aims to reduce boiler plate code and method name mentions. Uses name argument
 * as a string and a template argument.
 * @param name method name
 * @return result of method invocation
 */
#define REGISTER_HOST_API_FUNC(name) \
  imports_[#name] =                  \
      &importCall<&host_api::HostApi ::name>  // hack to make macro call look
                                              // natural by ending with ';'

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::runtime::binaryen {

  static const wasm::Name env = "env";
  /**
   * @note: some implementation details were taken from
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h
   */

  void RuntimeExternalInterface::methodsRegistration() {
    /// memory externals
    REGISTER_HOST_API_FUNC(ext_default_child_storage_root_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_root_version_2);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_set_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_get_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_clear_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_clear_prefix_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_clear_prefix_version_2);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_next_key_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_storage_kill_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_storage_kill_version_3);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_read_version_1);
    REGISTER_HOST_API_FUNC(ext_default_child_storage_exists_version_1);
    REGISTER_HOST_API_FUNC(ext_logging_log_version_1);
    REGISTER_HOST_API_FUNC(ext_logging_max_level_version_1);

    // -------------------------- crypto functions ---------------------------

    REGISTER_HOST_API_FUNC(ext_crypto_start_batch_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_finish_batch_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ed25519_public_keys_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ed25519_generate_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ed25519_sign_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ed25519_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ed25519_batch_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_sr25519_public_keys_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_sr25519_generate_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_sr25519_sign_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_sr25519_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_sr25519_verify_version_2);
    REGISTER_HOST_API_FUNC(ext_crypto_sr25519_batch_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_public_keys_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_sign_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_sign_prehashed_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_generate_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_verify_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_verify_version_2);
    REGISTER_HOST_API_FUNC(ext_crypto_ecdsa_verify_prehashed_version_1);
    /**
     *  secp256k1 recovery algorithms version_1 and version_2
     * are not different for our bitcoin secp256k1 library.
     * They have difference only for rust implementation and it
     * is use of `parse_standard` instead of
     * `parse_overflowing`. In comment, @see
     * https://github.com/paritytech/libsecp256k1/blob/d2ca104ea2cbda8f0708a6d80eb1da63e0cc0e69/src/lib.rs#L461
     * it is said that `parse_overflowing` is implementation
     * specific and won't be used in any other standard
     * libraries
     */
    REGISTER_HOST_API_FUNC(ext_crypto_secp256k1_ecdsa_recover_version_1);
    REGISTER_HOST_API_FUNC(ext_crypto_secp256k1_ecdsa_recover_version_2);
    REGISTER_HOST_API_FUNC(
        ext_crypto_secp256k1_ecdsa_recover_compressed_version_1);
    REGISTER_HOST_API_FUNC(
        ext_crypto_secp256k1_ecdsa_recover_compressed_version_2);

    // -------------------------- hashing functions --------------------------

    REGISTER_HOST_API_FUNC(ext_hashing_keccak_256_version_1);
    REGISTER_HOST_API_FUNC(ext_hashing_sha2_256_version_1);
    REGISTER_HOST_API_FUNC(ext_hashing_blake2_128_version_1);
    REGISTER_HOST_API_FUNC(ext_hashing_blake2_256_version_1);
    REGISTER_HOST_API_FUNC(ext_hashing_twox_256_version_1);
    REGISTER_HOST_API_FUNC(ext_hashing_twox_128_version_1);
    REGISTER_HOST_API_FUNC(ext_hashing_twox_64_version_1);

    // -------------------------- memory functions ---------------------------

    REGISTER_HOST_API_FUNC(ext_allocator_malloc_version_1);
    REGISTER_HOST_API_FUNC(ext_allocator_free_version_1);

    // -------------------------- storage functions --------------------------

    REGISTER_HOST_API_FUNC(ext_storage_set_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_get_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_clear_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_exists_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_read_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_clear_prefix_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_clear_prefix_version_2);
    REGISTER_HOST_API_FUNC(ext_storage_root_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_root_version_2);
    REGISTER_HOST_API_FUNC(ext_storage_changes_root_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_next_key_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_append_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_start_transaction_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_commit_transaction_version_1);
    REGISTER_HOST_API_FUNC(ext_storage_rollback_transaction_version_1);
    REGISTER_HOST_API_FUNC(ext_trie_blake2_256_root_version_1);
    REGISTER_HOST_API_FUNC(ext_trie_blake2_256_ordered_root_version_1);
    REGISTER_HOST_API_FUNC(ext_trie_blake2_256_ordered_root_version_2);
    REGISTER_HOST_API_FUNC(ext_misc_print_hex_version_1);
    REGISTER_HOST_API_FUNC(ext_misc_print_num_version_1);
    REGISTER_HOST_API_FUNC(ext_misc_print_utf8_version_1);
    REGISTER_HOST_API_FUNC(ext_misc_runtime_version_version_1);

    // ------------------------- Offchain extension --------------------------

    REGISTER_HOST_API_FUNC(ext_offchain_is_validator_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_submit_transaction_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_network_state_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_timestamp_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_sleep_until_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_random_seed_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_local_storage_set_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_local_storage_clear_version_1);
    REGISTER_HOST_API_FUNC(
        ext_offchain_local_storage_compare_and_set_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_local_storage_get_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_http_request_start_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_http_request_add_header_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_http_request_write_body_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_http_response_wait_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_http_response_headers_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_http_response_read_body_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_set_authorized_nodes_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_index_set_version_1);
    REGISTER_HOST_API_FUNC(ext_offchain_index_clear_version_1);
  }

  RuntimeExternalInterface::RuntimeExternalInterface(
      std::shared_ptr<host_api::HostApi> host_api)
      : host_api_{std::move(host_api)},
        logger_{log::createLogger("RuntimeExternalInterface", "binaryen")} {
    memory.resize(kInitialMemorySize);
    BOOST_ASSERT(host_api_);
    methodsRegistration();
  }

  RuntimeExternalInterface::InternalMemory *
  RuntimeExternalInterface::getMemory() {
    return &memory;
  }

  wasm::Literal RuntimeExternalInterface::callImport(
      wasm::Function *import, wasm::LiteralList &arguments) {
    SL_TRACE(logger_, "Call import {}", import->base.str);
    if (import->module == env) {
      auto it = imports_.find(
          import->base.c_str(), imports_.hash_function(), imports_.key_eq());
      if (it != imports_.end()) {
        return it->second(*this, import, arguments);
      }
    }

    wasm::Fatal() << "callImport: unknown import: " << import->module.str << "."
                  << import->name.str;
    return wasm::Literal();
  }

  void RuntimeExternalInterface::checkArguments(std::string_view extern_name,
                                                size_t expected,
                                                size_t actual) {
    if (expected != actual) {
      logger_->error(
          "Wrong number of arguments in {}. Expected: {}. Actual: {}",
          extern_name,
          expected,
          actual);
      throw std::runtime_error(
          "Invocation of a Host API method with wrong number of arguments");
    }
  }

  template <auto mf>
  wasm::Literal RuntimeExternalInterface::importCall(
      RuntimeExternalInterface &this_,
      wasm::Function *import,
      wasm::LiteralList &arguments) {
    this_.checkArguments(
        import->base.c_str(), hostApiFuncArgSize<mf>(), arguments.size());
    return callHostApiFunc<mf>(this_.host_api_.get(), arguments);
  }

  void RuntimeExternalInterface::init(wasm::Module &wasm,
                                      wasm::ModuleInstance &instance) {
    memory.resize(wasm.memory.initial * wasm::Memory::kPageSize);
    // apply memory segments
    for (auto &segment : wasm.memory.segments) {
      wasm::Address offset =
          (uint32_t)wasm::ConstantExpressionRunner<wasm::TrivialGlobalManager>(
              instance.globals)
              .visit(segment.offset)
              .value.geti32();
      if (offset + segment.data.size()
          > wasm.memory.initial * wasm::Memory::kPageSize) {
        trap("invalid offset when initializing memory");
      }
      for (size_t i = 0; i != segment.data.size(); ++i) {
        memory.set(offset + i, segment.data[i]);
      }
    }

    table.resize(wasm.table.initial);
    for (auto &segment : wasm.table.segments) {
      wasm::Address offset =
          (uint32_t)wasm::ConstantExpressionRunner<wasm::TrivialGlobalManager>(
              instance.globals)
              .visit(segment.offset)
              .value.geti32();
      if (offset + segment.data.size() > wasm.table.initial) {
        trap("invalid offset when initializing table");
      }
      for (size_t i = 0; i != segment.data.size(); ++i) {
        table[offset + i] = segment.data[i];
      }
    }
  }

  void RuntimeExternalInterface::importGlobals(
      std::map<wasm::Name, wasm::Literal> &globals, wasm::Module &wasm) {
    // add spectest globals
    wasm::ModuleUtils::iterImportedGlobals(wasm, [&](wasm::Global *import) {
      if (import->module == wasm::SPECTEST && import->base == wasm::GLOBAL) {
        switch (import->type) {
          case wasm::i32:
            globals[import->name] = wasm::Literal(int32_t(666));
            break;
          case wasm::i64:
            globals[import->name] = wasm::Literal(int64_t(666));
            break;
          case wasm::f32:
            globals[import->name] = wasm::Literal(float(666.6));
            break;
          case wasm::f64:
            globals[import->name] = wasm::Literal(double(666.6));
            break;
          case wasm::v128:
            assert(false && "v128 not implemented yet");
          case wasm::none:
          case wasm::unreachable:
            WASM_UNREACHABLE();
        }
      }
    });
    if (wasm.memory.imported() && wasm.memory.module == wasm::SPECTEST
        && wasm.memory.base == wasm::MEMORY) {
      // imported memory has initial 1 and max 2
      wasm.memory.initial = 1;
      wasm.memory.max = 2;
    }
  }

  wasm::Literal RuntimeExternalInterface::callTable(
      wasm::Index index,
      wasm::LiteralList &arguments,
      wasm::Type result,
      wasm::ModuleInstance &instance) {
    if (index >= table.size()) {
      trap("callTable overflow");
    }
    auto *func = instance.wasm.getFunctionOrNull(table[index]);
    if (!func) {
      trap("uninitialized table element");
    }
    if (func->params.size() != arguments.size()) {
      trap("callIndirect: bad # of arguments");
    }
    for (size_t i = 0; i < func->params.size(); i++) {
      if (func->params[i] != arguments[i].type) {
        trap("callIndirect: bad argument type");
      }
    }
    if (func->result != result) {
      trap("callIndirect: bad result type");
    }
    if (func->imported()) {
      return callImport(func, arguments);
    } else {
      return instance.callFunctionInternal(func->name, arguments);
    }
  }

}  // namespace kagome::runtime::binaryen
