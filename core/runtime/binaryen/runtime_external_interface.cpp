/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_external_interface.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"

namespace kagome::runtime {
  class TrieStorageProvider;
}

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
  const static wasm::Name ext_logging_log_version_1 =
      "ext_logging_log_version_1";
  const static wasm::Name ext_logging_max_level_version_1 =
      "ext_logging_max_level_version_1";
  const static wasm::Name ext_print_num = "ext_print_num";
  const static wasm::Name ext_print_utf8 = "ext_print_utf8";

  const static wasm::Name ext_blake2_128 = "ext_blake2_128";
  const static wasm::Name ext_blake2_256 = "ext_blake2_256";
  const static wasm::Name ext_keccak_256 = "ext_keccak_256";

  const static wasm::Name ext_start_batch_verify = "ext_start_batch_verify";
  const static wasm::Name ext_finish_batch_verify = "ext_finish_batch_verify";

  const static wasm::Name ext_ed25519_verify = "ext_ed25519_verify";
  const static wasm::Name ext_sr25519_verify = "ext_sr25519_verify";

  const static wasm::Name ext_twox_64 = "ext_twox_64";
  const static wasm::Name ext_twox_128 = "ext_twox_128";
  const static wasm::Name ext_twox_256 = "ext_twox_256";

  const static wasm::Name ext_chain_id = "ext_chain_id";

  const static wasm::Name ext_misc_print_hex_version_1 =
      "ext_misc_print_hex_version_1";
  const static wasm::Name ext_misc_print_num_version_1 =
      "ext_misc_print_num_version_1";
  const static wasm::Name ext_misc_print_utf8_version_1 =
      "ext_misc_print_utf8_version_1";
  const static wasm::Name ext_misc_runtime_version_version_1 =
      "ext_misc_runtime_version_version_1";

  // version 1
  const static wasm::Name ext_hashing_keccak_256_version_1 =
      "ext_hashing_keccak_256_version_1";
  const static wasm::Name ext_hashing_sha2_256_version_1 =
      "ext_hashing_sha2_256_version_1";
  const static wasm::Name ext_hashing_blake2_128_version_1 =
      "ext_hashing_blake2_128_version_1";
  const static wasm::Name ext_hashing_blake2_256_version_1 =
      "ext_hashing_blake2_256_version_1";
  const static wasm::Name ext_hashing_twox_256_version_1 =
      "ext_hashing_twox_256_version_1";
  const static wasm::Name ext_hashing_twox_128_version_1 =
      "ext_hashing_twox_128_version_1";
  const static wasm::Name ext_hashing_twox_64_version_1 =
      "ext_hashing_twox_64_version_1";

  const static wasm::Name ext_allocator_malloc_version_1 =
      "ext_allocator_malloc_version_1";
  const static wasm::Name ext_allocator_free_version_1 =
      "ext_allocator_free_version_1";

  const static wasm::Name ext_storage_set_version_1 =
      "ext_storage_set_version_1";
  const static wasm::Name ext_storage_get_version_1 =
      "ext_storage_get_version_1";
  const static wasm::Name ext_storage_clear_version_1 =
      "ext_storage_clear_version_1";
  const static wasm::Name ext_storage_exists_version_1 =
      "ext_storage_exists_version_1";
  const static wasm::Name ext_storage_read_version_1 =
      "ext_storage_read_version_1";
  const static wasm::Name ext_storage_clear_prefix_version_1 =
      "ext_storage_clear_prefix_version_1";
  const static wasm::Name ext_storage_root_version_1 =
      "ext_storage_root_version_1";
  const static wasm::Name ext_storage_changes_root_version_1 =
      "ext_storage_changes_root_version_1";
  const static wasm::Name ext_storage_next_key_version_1 =
      "ext_storage_next_key_version_1";
  const static wasm::Name ext_storage_append_version_1 =
      "ext_storage_append_version_1";

  const static wasm::Name ext_crypto_start_batch_verify_version_1 =
      "ext_crypto_start_batch_verify_version_1";
  const static wasm::Name ext_crypto_finish_batch_verify_version_1 =
      "ext_crypto_finish_batch_verify_version_1";

  const static wasm::Name ext_crypto_ed25519_public_keys_version_1 =
      "ext_crypto_ed25519_public_keys_version_1";
  const static wasm::Name ext_crypto_ed25519_generate_version_1 =
      "ext_crypto_ed25519_generate_version_1";
  const static wasm::Name ext_crypto_ed25519_sign_version_1 =
      "ext_crypto_ed25519_sign_version_1";
  const static wasm::Name ext_crypto_ed25519_verify_version_1 =
      "ext_crypto_ed25519_verify_version_1";

  const static wasm::Name ext_crypto_sr25519_public_keys_version_1 =
      "ext_crypto_sr25519_public_keys_version_1";
  const static wasm::Name ext_crypto_sr25519_generate_version_1 =
      "ext_crypto_sr25519_generate_version_1";
  const static wasm::Name ext_crypto_sr25519_sign_version_1 =
      "ext_crypto_sr25519_sign_version_1";
  const static wasm::Name ext_crypto_sr25519_verify_version_1 =
      "ext_crypto_sr25519_verify_version_1";
  const static wasm::Name ext_crypto_sr25519_verify_version_2 =
      "ext_crypto_sr25519_verify_version_2";

  const static wasm::Name ext_crypto_secp256k1_ecdsa_recover_version_1 =
      "ext_crypto_secp256k1_ecdsa_recover_version_1";
  const static wasm::Name
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_1 =
          "ext_crypto_secp256k1_ecdsa_recover_compressed_version_1";

  const static wasm::Name ext_trie_blake2_256_root_version_1 =
      "ext_trie_blake2_256_root_version_1";
  const static wasm::Name ext_trie_blake2_256_ordered_root_version_1 =
      "ext_trie_blake2_256_ordered_root_version_1";

  /**
   * @note: some implementation details were taken from
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h
   */

  RuntimeExternalInterface::RuntimeExternalInterface(
      std::shared_ptr<CoreFactory> core_factory,
      std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory,
      std::shared_ptr<BinaryenWasmMemoryFactory> wasm_memory_factory,
      const std::shared_ptr<host_api::HostApiFactory> &host_api_factory,
      std::shared_ptr<TrieStorageProvider> storage_provider) {
    BOOST_ASSERT_MSG(wasm_memory_factory != nullptr,
                     "wasm memory factory is nullptr");
    BOOST_ASSERT_MSG(host_api_factory != nullptr,
                     "host api factory is nullptr");
    BOOST_ASSERT_MSG(storage_provider != nullptr,
                     "storage provider is nullptr");
    host_api_ = host_api_factory->make(
        core_factory,
        runtime_env_factory,
        wasm_memory_factory->make(&(ShellExternalInterface::memory)),
        std::move(storage_provider));
  }

  wasm::Literal RuntimeExternalInterface::callImport(
      wasm::Function *import, wasm::LiteralList &arguments) {
    SL_TRACE(logger_, "Call import {}", import->base);
    // TODO(kamilsa): PRE-359 Replace ifs with switch case
    if (import->module == env) {
      /// memory externals
      /// ext_malloc
      if (import->base == ext_malloc) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto ptr = host_api_->ext_malloc(arguments.at(0).geti32());
        return wasm::Literal(ptr);
      }
      /// ext_free
      if (import->base == ext_free) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_free(arguments.at(0).geti32());
        return wasm::Literal();
      }
      /// storage externals

      /// ext_clear_prefix
      if (import->base == ext_clear_prefix) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_clear_prefix(arguments.at(0).geti32(),
                                    arguments.at(1).geti32());
        return wasm::Literal();
      }
      /// ext_clear_storage
      if (import->base == ext_clear_storage) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_clear_storage(arguments.at(0).geti32(),
                                     arguments.at(1).geti32());
        return wasm::Literal();
      }
      /// ext_exists_storage
      if (import->base == ext_exists_storage) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto storage_exists = host_api_->ext_exists_storage(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(storage_exists);
      }
      /// ext_get_allocated_storage
      if (import->base == ext_get_allocated_storage) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto ptr =
            host_api_->ext_get_allocated_storage(arguments.at(0).geti32(),
                                                 arguments.at(1).geti32(),
                                                 arguments.at(2).geti32());
        return wasm::Literal(ptr);
      }
      /// ext_get_storage_into
      if (import->base == ext_get_storage_into) {
        checkArguments(import->base.c_str(), 5, arguments.size());
        auto res = host_api_->ext_get_storage_into(arguments.at(0).geti32(),
                                                   arguments.at(1).geti32(),
                                                   arguments.at(2).geti32(),
                                                   arguments.at(3).geti32(),
                                                   arguments.at(4).geti32());
        return wasm::Literal(res);
      }
      /// ext_storage_read_version_1
      if (import->base == ext_storage_read_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res =
            host_api_->ext_storage_read_version_1(arguments.at(0).geti64(),
                                                  arguments.at(1).geti64(),
                                                  arguments.at(2).geti32());
        return wasm::Literal(res);
      }
      /// ext_set_storage
      if (import->base == ext_set_storage) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        host_api_->ext_set_storage(arguments.at(0).geti32(),
                                   arguments.at(1).geti32(),
                                   arguments.at(2).geti32(),
                                   arguments.at(3).geti32());
        return wasm::Literal();
      }
      /// ext_blake2_256_enumerated_trie_root
      if (import->base == ext_blake2_256_enumerated_trie_root) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        host_api_->ext_blake2_256_enumerated_trie_root(
            arguments.at(0).geti32(),
            arguments.at(1).geti32(),
            arguments.at(2).geti32(),
            arguments.at(3).geti32());
        return wasm::Literal();
      }
      /// ext_storage_changes_root
      if (import->base == ext_storage_changes_root) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_storage_changes_root(
            arguments.at(0).geti32(), arguments.at(2).geti32());
        return wasm::Literal(res);
      }
      /// ext_storage_root
      if (import->base == ext_storage_root) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_storage_root(arguments.at(0).geti32());
        return wasm::Literal();
      }

      /// IO extensions

      /// ext_print_hex
      if (import->base == ext_print_hex) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_print_hex(arguments.at(0).geti32(),
                                 arguments.at(1).geti32());
        return wasm::Literal();
      }
      /// ext_logging_log_version_1
      if (import->base == ext_logging_log_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_logging_log_version_1(arguments.at(0).geti32(),
                                             arguments.at(1).geti64(),
                                             arguments.at(2).geti64());
        return wasm::Literal();
      }
      /// ext_logging_max_level_version_1
      if(import->base == ext_logging_max_level_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_logging_max_level_version_1();
        return wasm::Literal(res);
      }
      /// ext_print_num
      if (import->base == ext_print_num) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_print_num(arguments.at(0).geti64());
        return wasm::Literal();
      }
      /// ext_print_utf8
      if (import->base == ext_print_utf8) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_print_utf8(arguments.at(0).geti32(),
                                  arguments.at(1).geti32());
        return wasm::Literal();
      }

      /// Cryptographic extensions
      /// ext_blake2_128
      if (import->base == ext_blake2_128) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_blake2_128(arguments.at(0).geti32(),
                                  arguments.at(1).geti32(),
                                  arguments.at(2).geti32());
        return wasm::Literal();
      }

      /// ext_blake2_256
      if (import->base == ext_blake2_256) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_blake2_256(arguments.at(0).geti32(),
                                  arguments.at(1).geti32(),
                                  arguments.at(2).geti32());
        return wasm::Literal();
      }

      /// ext_keccak_256
      if (import->base == ext_keccak_256) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_keccak_256(arguments.at(0).geti32(),
                                  arguments.at(1).geti32(),
                                  arguments.at(2).geti32());
        return wasm::Literal();
      }

      // ext_start_batch_verify
      if (import->base == ext_start_batch_verify) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        host_api_->ext_start_batch_verify();
        return wasm::Literal();
      }

      // ext_finish_batch_verify
      if (import->base == ext_finish_batch_verify) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_finish_batch_verify();
        return wasm::Literal(res);
      }

      /// ext_ed25519_verify
      if (import->base == ext_ed25519_verify) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        auto res = host_api_->ext_ed25519_verify(arguments.at(0).geti32(),
                                                 arguments.at(1).geti32(),
                                                 arguments.at(2).geti32(),
                                                 arguments.at(3).geti32());
        return wasm::Literal(res);
      }
      /// ext_sr25519_verify
      if (import->base == ext_sr25519_verify) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        auto res = host_api_->ext_sr25519_verify(arguments.at(0).geti32(),
                                                 arguments.at(1).geti32(),
                                                 arguments.at(2).geti32(),
                                                 arguments.at(3).geti32());
        return wasm::Literal(res);
      }
      /// ext_twox_64
      if (import->base == ext_twox_64) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_twox_64(arguments.at(0).geti32(),
                               arguments.at(1).geti32(),
                               arguments.at(2).geti32());
        return wasm::Literal();
      }
      /// ext_twox_128
      if (import->base == ext_twox_128) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_twox_128(arguments.at(0).geti32(),
                                arguments.at(1).geti32(),
                                arguments.at(2).geti32());
        return wasm::Literal();
      }
      /// ext_twox_256
      if (import->base == ext_twox_256) {
        checkArguments(import->base.c_str(), 3, arguments.size());

        host_api_->ext_twox_256(arguments.at(0).geti32(),
                                arguments.at(1).geti32(),
                                arguments.at(2).geti32());
        return wasm::Literal();
      }
      /// ext_chain_id
      if (import->base == ext_chain_id) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_chain_id();
        return wasm::Literal(res);
      }

      // ----------------------- api version 1 ---------------------------

      // ----------------------- crypto functions ------------------------

      // ext_crypto_start_batch_verify_version_1
      if (import->base == ext_crypto_start_batch_verify_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        host_api_->ext_start_batch_verify();
        return wasm::Literal();
      }

      // ext_crypto_finish_batch_verify_version_1
      if (import->base == ext_crypto_finish_batch_verify_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_finish_batch_verify();
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_public_keys_version_1
      if (import->base == ext_crypto_ed25519_public_keys_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_ed25519_public_keys_v1(arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_generate_version_1
      if (import->base == ext_crypto_ed25519_generate_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_ed25519_generate_v1(arguments.at(0).geti32(),
                                                      arguments.at(1).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_sign_version_1
      if (import->base == ext_crypto_ed25519_sign_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_ed25519_sign_v1(arguments.at(0).geti32(),
                                                  arguments.at(1).geti32(),
                                                  arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_verify_version_1
      if (import->base == ext_crypto_ed25519_verify_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_ed25519_verify_v1(arguments.at(0).geti32(),
                                                    arguments.at(1).geti64(),
                                                    arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_public_keys_version_1
      if (import->base == ext_crypto_sr25519_public_keys_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_sr25519_public_keys_v1(arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_generate_version_1
      if (import->base == ext_crypto_sr25519_generate_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_sr25519_generate_v1(arguments.at(0).geti32(),
                                                      arguments.at(1).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_sign_version_1
      if (import->base == ext_crypto_sr25519_sign_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_sr25519_sign_v1(arguments.at(0).geti32(),
                                                  arguments.at(1).geti32(),
                                                  arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_verify_version_1
      if (import->base == ext_crypto_sr25519_verify_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_sr25519_verify_v1(arguments.at(0).geti32(),
                                                    arguments.at(1).geti64(),
                                                    arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_verify_version_2
      if (import->base == ext_crypto_sr25519_verify_version_2) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_sr25519_verify_v1(arguments.at(0).geti32(),
                                                    arguments.at(1).geti64(),
                                                    arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_secp256k1_ecdsa_recover_version_1
      if (import->base == ext_crypto_secp256k1_ecdsa_recover_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_crypto_secp256k1_ecdsa_recover_v1(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_secp256k1_ecdsa_recover_compressed_version_1
      if (import->base
          == ext_crypto_secp256k1_ecdsa_recover_compressed_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(res);
      }

      // ----------------------- hashing functions ------------------------

      /// ext_hashing_keccak_256_version_1
      if (import->base == ext_hashing_keccak_256_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_hashing_keccak_256_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_hashing_sha2_256_version_1
      if (import->base == ext_hashing_sha2_256_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_hashing_sha2_256_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_hashing_blake2_128_version_1
      if (import->base == ext_hashing_blake2_128_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_hashing_blake2_128_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_hashing_blake2_256_version_1
      if (import->base == ext_hashing_blake2_256_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_hashing_blake2_256_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_hashing_twox_256_version_1
      if (import->base == ext_hashing_twox_256_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_hashing_twox_256_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_hashing_twox_128_version_1
      if (import->base == ext_hashing_twox_128_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_hashing_twox_128_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_hashing_twox_64_version_1
      if (import->base == ext_hashing_twox_64_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_hashing_twox_64_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_allocator_malloc_version_1
      if (import->base == ext_allocator_malloc_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_allocator_malloc_version_1(arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      // ----------------------- memory functions ------------------------

      /// ext_allocator_malloc_version_1
      if (import->base == ext_allocator_malloc_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_allocator_malloc_version_1(arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      /// ext_allocator_free_version_1
      if (import->base == ext_allocator_free_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_allocator_free_version_1(arguments.at(0).geti32());
        return wasm::Literal();
      }

      // ----------------------- storage functions ------------------------
      /// ext_storage_set_version_1
      if (import->base == ext_storage_set_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_storage_set_version_1(arguments.at(0).geti64(),
                                             arguments.at(1).geti64());
        return wasm::Literal();
      }

      /// ext_storage_get_version_1
      if (import->base == ext_storage_get_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_storage_get_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_storage_clear_version_1
      if (import->base == ext_storage_clear_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_storage_clear_version_1(arguments.at(0).geti64());
        return wasm::Literal();
      }

      /// ext_storage_exists_version_1
      if (import->base == ext_storage_exists_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_storage_exists_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_storage_read_version_1
      if (import->base == ext_storage_read_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res =
            host_api_->ext_storage_read_version_1(arguments.at(0).geti64(),
                                                  arguments.at(1).geti64(),
                                                  arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /// ext_storage_clear_prefix_version_1
      if (import->base == ext_storage_clear_prefix_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_storage_clear_prefix_version_1(arguments.at(0).geti64());
        return wasm::Literal();
      }

      /// ext_storage_root_version_1
      if (import->base == ext_storage_root_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_storage_root_version_1();
        return wasm::Literal(res);
      }

      /// ext_storage_changes_root_version_1
      if (import->base == ext_storage_changes_root_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_storage_changes_root_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_storage_next_key_version_1
      if (import->base == ext_storage_next_key_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res =
            host_api_->ext_storage_next_key_version_1(arguments.at(0).geti64());
        return wasm::Literal(res);
      }
      /// ext_storage_append_version_1
      if (import->base == ext_storage_append_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_storage_append_version_1(arguments.at(0).geti64(),
                                                arguments.at(1).geti64());
        return wasm::Literal();
      }

      /// ext_trie_blake2_256_root_version_1
      if (import->base == ext_trie_blake2_256_root_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_trie_blake2_256_root_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_trie_blake2_256_ordered_root_version_1
      if (import->base == ext_trie_blake2_256_ordered_root_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_trie_blake2_256_ordered_root_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_misc_print_hex_version_1
      if (import->base == ext_misc_print_hex_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_misc_print_hex_version_1(arguments.at(0).geti64());
        return wasm::Literal();
      }

      /// ext_misc_print_num_version_1
      if (import->base == ext_misc_print_num_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_misc_print_num_version_1(arguments.at(0).geti64());
        return wasm::Literal();
      }

      /// ext_misc_print_utf8_version_1
      if (import->base == ext_misc_print_utf8_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_misc_print_utf8_version_1(arguments.at(0).geti64());
        return wasm::Literal();
      }

      /// ext_misc_runtime_version_version_1
      if (import->base == ext_misc_runtime_version_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_misc_runtime_version_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res.combine());
      }

      // TODO(xDimon): It is temporary suppress fails at calling of
      //  callImport(ext_offchain_index_set_version_1)
      if (import->base == "ext_offchain_index_set_version_1") {
        return wasm::Literal();
      }
    }

    wasm::Fatal() << "callImport: unknown import: " << import->module.str << "."
                  << import->name.str;
  }  // namespace kagome::runtime::binaryen

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

  std::shared_ptr<WasmMemory> RuntimeExternalInterface::memory() const {
    return host_api_->memory();
  }

  void RuntimeExternalInterface::reset() const {
    return host_api_->reset();
  }

}  // namespace kagome::runtime::binaryen
