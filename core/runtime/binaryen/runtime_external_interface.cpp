/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_external_interface.hpp"

#include "runtime/binaryen/wasm_memory_impl.hpp"

namespace kagome::runtime::binaryen {

  const static wasm::Name env = "env";

  const static wasm::Name ext_malloc = "ext_malloc";
  const static wasm::Name ext_free = "ext_free";

  const static wasm::Name ext_clear_prefix = "ext_clear_prefix";
  const static wasm::Name ext_clear_storage = "ext_clear_storage";
  const static wasm::Name ext_exists_storage = "ext_exists_storage";
  const static wasm::Name ext_get_allocated_storage =
      "ext_get_allocated_storage";
  const static wasm::Name ext_get_storage_into = "ext_get_storage_into";
  const static wasm::Name ext_set_storage = "ext_set_storage";
  const static wasm::Name ext_blake2_256_enumerated_trie_root =
      "ext_blake2_256_enumerated_trie_root";
  const static wasm::Name ext_storage_changes_root = "ext_storage_changes_root";
  const static wasm::Name ext_storage_root = "ext_storage_root";
  const static wasm::Name ext_child_storage_root = "ext_child_storage_root";
  const static wasm::Name ext_clear_child_storage = "ext_clear_child_storage";
  const static wasm::Name ext_exists_child_storage = "ext_exists_child_storage";
  const static wasm::Name ext_get_allocated_child_storage =
      "ext_get_allocated_child_storage";
  const static wasm::Name ext_get_child_storage_into =
      "ext_get_child_storage_into";
  const static wasm::Name ext_kill_child_storage = "ext_kill_child_storage";
  const static wasm::Name ext_set_child_storage = "ext_set_child_storage";

  const static wasm::Name ext_print_hex = "ext_print_hex";
  const static wasm::Name ext_print_num = "ext_print_num";
  const static wasm::Name ext_print_utf8 = "ext_print_utf8";

  const static wasm::Name ext_blake2_128 = "ext_blake2_128";
  const static wasm::Name ext_blake2_256 = "ext_blake2_256";
  const static wasm::Name ext_keccak_256 = "ext_keccak_256";
  const static wasm::Name ext_ed25519_verify = "ext_ed25519_verify";
  const static wasm::Name ext_sr25519_verify = "ext_sr25519_verify";
  const static wasm::Name ext_twox_64 = "ext_twox_64";
  const static wasm::Name ext_twox_128 = "ext_twox_128";
  const static wasm::Name ext_twox_256 = "ext_twox_256";

  const static wasm::Name ext_secp256k1_ecdsa_recover_v1 =
      "ext_crypto_secp256k1_ecdsa_recover_version_1";
  const static wasm::Name ext_secp256k1_ecdsa_recover_compressed_v1 =
      "ext_crypto_secp256k1_ecdsa_recover_compressed_version_1";

  const static wasm::Name ext_chain_id = "ext_chain_id";
  /**
   * @note: some implementation details were taken from
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h
   */

  RuntimeExternalInterface::RuntimeExternalInterface(
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory,
      std::shared_ptr<TrieStorageProvider> storage_provider) {
    BOOST_ASSERT_MSG(extension_factory != nullptr,
                     "extension factory is nullptr");
    BOOST_ASSERT_MSG(storage_provider != nullptr,
                     "storage provider is nullptr");
    auto memory_impl =
        std::make_shared<WasmMemoryImpl>(&(ShellExternalInterface::memory));
    extension_ = extension_factory->createExtension(
        memory_impl, std::move(storage_provider));
  }

  wasm::Literal RuntimeExternalInterface::callImport(
      wasm::Function *import, wasm::LiteralList &arguments) {
    // TODO(kamilsa): PRE-359 Replace ifs with switch case
    if (import->module == env) {
      /// memory externals
      /// ext_malloc
      if (import->base == ext_malloc) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto ptr = extension_->ext_malloc(arguments.at(0).geti32());
        return wasm::Literal(ptr);
      }
      /// ext_free
      if (import->base == ext_free) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        extension_->ext_free(arguments.at(0).geti32());
        return wasm::Literal();
      }
      /// storage externals

      /// ext_clear_prefix
      if (import->base == ext_clear_prefix) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        extension_->ext_clear_prefix(arguments.at(0).geti32(),
                                     arguments.at(1).geti32());
        return wasm::Literal();
      }
      /// ext_clear_storage
      if (import->base == ext_clear_storage) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        extension_->ext_clear_storage(arguments.at(0).geti32(),
                                      arguments.at(1).geti32());
        return wasm::Literal();
      }
      /// ext_exists_storage
      if (import->base == ext_exists_storage) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto storage_exists = extension_->ext_exists_storage(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(storage_exists);
      }
      /// ext_get_allocated_storage
      if (import->base == ext_get_allocated_storage) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto ptr =
            extension_->ext_get_allocated_storage(arguments.at(0).geti32(),
                                                  arguments.at(1).geti32(),
                                                  arguments.at(2).geti32());
        return wasm::Literal(ptr);
      }
      /// ext_get_storage_into
      if (import->base == ext_get_storage_into) {
        checkArguments(import->base.c_str(), 5, arguments.size());
        auto res = extension_->ext_get_storage_into(arguments.at(0).geti32(),
                                                    arguments.at(1).geti32(),
                                                    arguments.at(2).geti32(),
                                                    arguments.at(3).geti32(),
                                                    arguments.at(4).geti32());
        return wasm::Literal(res);
      }
      /// ext_set_storage
      if (import->base == ext_set_storage) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        extension_->ext_set_storage(arguments.at(0).geti32(),
                                    arguments.at(1).geti32(),
                                    arguments.at(2).geti32(),
                                    arguments.at(3).geti32());
        return wasm::Literal();
      }
      /// ext_blake2_256_enumerated_trie_root
      if (import->base == ext_blake2_256_enumerated_trie_root) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        extension_->ext_blake2_256_enumerated_trie_root(
            arguments.at(0).geti32(),
            arguments.at(1).geti32(),
            arguments.at(2).geti32(),
            arguments.at(3).geti32());
        return wasm::Literal();
      }
      /// ext_storage_changes_root
      if (import->base == ext_storage_changes_root) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = extension_->ext_storage_changes_root(
            arguments.at(0).geti32(), arguments.at(2).geti32());
        return wasm::Literal(res);
      }
      /// ext_storage_root
      if (import->base == ext_storage_root) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        extension_->ext_storage_root(arguments.at(0).geti32());
        return wasm::Literal();
      }

      /// IO extensions

      /// ext_print_hex
      if (import->base == ext_print_hex) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        extension_->ext_print_hex(arguments.at(0).geti32(),
                                  arguments.at(1).geti32());
        return wasm::Literal();
      }
      /// ext_print_num
      if (import->base == ext_print_num) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        extension_->ext_print_num(arguments.at(0).geti64());
        return wasm::Literal();
      }
      /// ext_print_utf8
      if (import->base == ext_print_utf8) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        extension_->ext_print_utf8(arguments.at(0).geti32(),
                                   arguments.at(1).geti32());
        return wasm::Literal();
      }

      /// Cryptographuc extensions
      /// ext_blake2_128
      if (import->base == ext_blake2_128) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        extension_->ext_blake2_128(arguments.at(0).geti32(),
                                   arguments.at(1).geti32(),
                                   arguments.at(2).geti32());
        return wasm::Literal();
      }

      /// ext_blake2_256
      if (import->base == ext_blake2_256) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        extension_->ext_blake2_256(arguments.at(0).geti32(),
                                   arguments.at(1).geti32(),
                                   arguments.at(2).geti32());
        return wasm::Literal();
      }

      /// ext_keccak_256
      if (import->base == ext_keccak_256) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        extension_->ext_keccak_256(arguments.at(0).geti32(),
                                   arguments.at(1).geti32(),
                                   arguments.at(2).geti32());
        return wasm::Literal();
      }

      /// ext_ed25519_verify
      if (import->base == ext_ed25519_verify) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        auto res = extension_->ext_ed25519_verify(arguments.at(0).geti32(),
                                                  arguments.at(1).geti32(),
                                                  arguments.at(2).geti32(),
                                                  arguments.at(3).geti32());
        return wasm::Literal(res);
      }
      /// ext_sr25519_verify
      if (import->base == ext_sr25519_verify) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        auto res = extension_->ext_sr25519_verify(arguments.at(0).geti32(),
                                                  arguments.at(1).geti32(),
                                                  arguments.at(2).geti32(),
                                                  arguments.at(3).geti32());
        return wasm::Literal(res);
      }
      /// ext_twox_64
      if (import->base == ext_twox_64) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        extension_->ext_twox_64(arguments.at(0).geti32(),
                                arguments.at(1).geti32(),
                                arguments.at(2).geti32());
        return wasm::Literal();
      }
      /// ext_twox_128
      if (import->base == ext_twox_128) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        extension_->ext_twox_128(arguments.at(0).geti32(),
                                 arguments.at(1).geti32(),
                                 arguments.at(2).geti32());
        return wasm::Literal();
      }
      /// ext_twox_256
      if (import->base == ext_twox_256) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        extension_->ext_twox_256(arguments.at(0).geti32(),
                                 arguments.at(1).geti32(),
                                 arguments.at(2).geti32());
        return wasm::Literal();
      }
      /// ext_chain_id
      if (import->base == ext_chain_id) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = extension_->ext_chain_id();
        return wasm::Literal(res);
      }

      /// crypto version 1

      /// ext_secp256k1_ecdsa_recover_v1
      if (import->base == ext_secp256k1_ecdsa_recover_v1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = extension_->ext_crypto_secp256k1_ecdsa_recover_v1(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(res);
      }

      /// ext_secp256k1_ecdsa_recover_compressed_v1
      if (import->base == ext_secp256k1_ecdsa_recover_compressed_v1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = extension_->ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(res);
      }
    }
    wasm::Fatal() << "callImport: unknown import: " << import->module.str << "."
                  << import->name.str;
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
      std::terminate();
    }
  }

}  // namespace kagome::runtime::binaryen
