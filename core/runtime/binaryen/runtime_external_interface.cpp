/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_external_interface.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::runtime::binaryen {

  const static wasm::Name env = "env";

  const static wasm::Name ext_logging_log_version_1 =
      "ext_logging_log_version_1";
  const static wasm::Name ext_logging_max_level_version_1 =
      "ext_logging_max_level_version_1";

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
  const static wasm::Name ext_storage_clear_prefix_version_2 =
      "ext_storage_clear_prefix_version_2";
  const static wasm::Name ext_storage_root_version_1 =
      "ext_storage_root_version_1";
  const static wasm::Name ext_storage_changes_root_version_1 =
      "ext_storage_changes_root_version_1";
  const static wasm::Name ext_storage_next_key_version_1 =
      "ext_storage_next_key_version_1";
  const static wasm::Name ext_storage_append_version_1 =
      "ext_storage_append_version_1";

  const static wasm::Name ext_storage_start_transaction_version_1 =
      "ext_storage_start_transaction_version_1";
  const static wasm::Name ext_storage_commit_transaction_version_1 =
      "ext_storage_commit_transaction_version_1";
  const static wasm::Name ext_storage_rollback_transaction_version_1 =
      "ext_storage_rollback_transaction_version_1";

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
  const static wasm::Name ext_crypto_secp256k1_ecdsa_recover_version_2 =
      "ext_crypto_secp256k1_ecdsa_recover_version_2";
  const static wasm::Name
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_1 =
          "ext_crypto_secp256k1_ecdsa_recover_compressed_version_1";
  const static wasm::Name
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_2 =
          "ext_crypto_secp256k1_ecdsa_recover_compressed_version_2";

  const static wasm::Name ext_trie_blake2_256_root_version_1 =
      "ext_trie_blake2_256_root_version_1";
  const static wasm::Name ext_trie_blake2_256_ordered_root_version_1 =
      "ext_trie_blake2_256_ordered_root_version_1";

  // --------------------------- Offchain extension ----------------------------

  const static wasm::Name ext_offchain_is_validator_version_1 =
      "ext_offchain_is_validator_version_1";
  const static wasm::Name ext_offchain_submit_transaction_version_1 =
      "ext_offchain_submit_transaction_version_1";
  const static wasm::Name ext_offchain_network_state_version_1 =
      "ext_offchain_network_state_version_1";
  const static wasm::Name ext_offchain_timestamp_version_1 =
      "ext_offchain_timestamp_version_1";
  const static wasm::Name ext_offchain_sleep_until_version_1 =
      "ext_offchain_sleep_until_version_1";
  const static wasm::Name ext_offchain_random_seed_version_1 =
      "ext_offchain_random_seed_version_1";
  const static wasm::Name ext_offchain_local_storage_set_version_1 =
      "ext_offchain_local_storage_set_version_1";
  const static wasm::Name ext_offchain_local_storage_clear_version_1 =
      "ext_offchain_local_storage_clear_version_1";
  const static wasm::Name ext_offchain_local_storage_compare_and_set_version_1 =
      "ext_offchain_local_storage_compare_and_set_version_1";
  const static wasm::Name ext_offchain_local_storage_get_version_1 =
      "ext_offchain_local_storage_get_version_1";
  const static wasm::Name ext_offchain_http_request_start_version_1 =
      "ext_offchain_http_request_start_version_1";
  const static wasm::Name ext_offchain_http_request_add_header_version_1 =
      "ext_offchain_http_request_add_header_version_1";
  const static wasm::Name ext_offchain_http_request_write_body_version_1 =
      "ext_offchain_http_request_write_body_version_1";
  const static wasm::Name ext_offchain_http_response_wait_version_1 =
      "ext_offchain_http_response_wait_version_1";
  const static wasm::Name ext_offchain_http_response_headers_version_1 =
      "ext_offchain_http_response_headers_version_1";
  const static wasm::Name ext_offchain_http_response_read_body_version_1 =
      "ext_offchain_http_response_read_body_version_1";
  const static wasm::Name ext_offchain_set_authorized_nodes_version_1 =
      "ext_offchain_set_authorized_nodes_version_1";
  const static wasm::Name ext_offchain_index_set_version_1 =
      "ext_offchain_index_set_version_1";
  const static wasm::Name ext_offchain_index_clear_version_1 =
      "ext_offchain_index_clear_version_1";

  /**
   * @note: some implementation details were taken from
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h
   */

  RuntimeExternalInterface::RuntimeExternalInterface(
      std::shared_ptr<host_api::HostApi> host_api)
      : host_api_{std::move(host_api)},
        logger_{log::createLogger("RuntimeExternalInterface", "binaryen")} {
    memory.resize(kInitialMemorySize);
    BOOST_ASSERT(host_api_);
  }

  wasm::ShellExternalInterface::Memory *RuntimeExternalInterface::getMemory() {
    return &memory;
  }

  wasm::Literal RuntimeExternalInterface::callImport(
      wasm::Function *import, wasm::LiteralList &arguments) {
    SL_TRACE(logger_, "Call import {}", import->base);
    // TODO(kamilsa): PRE-359 Replace ifs with switch case
    if (import->module == env) {
      /// memory externals
      /// ext_storage_read_version_1
      if (import->base == ext_storage_read_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res =
            host_api_->ext_storage_read_version_1(arguments.at(0).geti64(),
                                                  arguments.at(1).geti64(),
                                                  arguments.at(2).geti32());
        return wasm::Literal(res);
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
      if (import->base == ext_logging_max_level_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_logging_max_level_version_1();
        return wasm::Literal(res);
      }
      // -------------------------- crypto functions ---------------------------

      // ext_crypto_start_batch_verify_version_1
      if (import->base == ext_crypto_start_batch_verify_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        host_api_->ext_crypto_start_batch_verify_version_1();
        return wasm::Literal();
      }

      // ext_crypto_finish_batch_verify_version_1
      if (import->base == ext_crypto_finish_batch_verify_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_crypto_finish_batch_verify_version_1();
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_public_keys_version_1
      if (import->base == ext_crypto_ed25519_public_keys_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_crypto_ed25519_public_keys_version_1(
            arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_generate_version_1
      if (import->base == ext_crypto_ed25519_generate_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_crypto_ed25519_generate_version_1(
            arguments.at(0).geti32(), arguments.at(1).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_sign_version_1
      if (import->base == ext_crypto_ed25519_sign_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_crypto_ed25519_sign_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti32(),
            arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_ed25519_verify_version_1
      if (import->base == ext_crypto_ed25519_verify_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_crypto_ed25519_verify_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti64(),
            arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_public_keys_version_1
      if (import->base == ext_crypto_sr25519_public_keys_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_crypto_sr25519_public_keys_version_1(
            arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_generate_version_1
      if (import->base == ext_crypto_sr25519_generate_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_crypto_sr25519_generate_version_1(
            arguments.at(0).geti32(), arguments.at(1).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_sign_version_1
      if (import->base == ext_crypto_sr25519_sign_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_crypto_sr25519_sign_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti32(),
            arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_verify_version_1
      if (import->base == ext_crypto_sr25519_verify_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_crypto_sr25519_verify_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti64(),
            arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_sr25519_verify_version_2
      if (import->base == ext_crypto_sr25519_verify_version_2) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_crypto_sr25519_verify_version_2(
            arguments.at(0).geti32(),
            arguments.at(1).geti64(),
            arguments.at(2).geti32());
        return wasm::Literal(res);
      }

      /**
       *  secp256k1 recovery algorithms version_1 and version_2 are not
       * different for our bitcoin secp256k1 library. They have difference only
       * for rust implementation and it is use of `parse_standard` instead of
       * `parse_overflowing`. In comment, @see
       * https://github.com/paritytech/libsecp256k1/blob/d2ca104ea2cbda8f0708a6d80eb1da63e0cc0e69/src/lib.rs#L461
       * it is said that `parse_overflowing` is implementation specific and
       * won't be used in any other standard libraries
       */
      /// ext_crypto_secp256k1_ecdsa_recover_version_1
      if (import->base == ext_crypto_secp256k1_ecdsa_recover_version_1
          || import->base == ext_crypto_secp256k1_ecdsa_recover_version_2) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_crypto_secp256k1_ecdsa_recover_version_1(
            arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(res);
      }

      /// ext_crypto_secp256k1_ecdsa_recover_compressed_version_1
      if (import->base
              == ext_crypto_secp256k1_ecdsa_recover_compressed_version_1
          || import->base
                 == ext_crypto_secp256k1_ecdsa_recover_compressed_version_2) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res =
            host_api_->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
                arguments.at(0).geti32(), arguments.at(1).geti32());
        return wasm::Literal(res);
      }

      // -------------------------- hashing functions --------------------------

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

      // -------------------------- memory functions ---------------------------

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

      // -------------------------- storage functions --------------------------
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

      /// ext_storage_clear_prefix_version_2
      if (import->base == ext_storage_clear_prefix_version_2) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_storage_clear_prefix_version_2(
            arguments.at(0).geti64(), arguments.at(1).geti64());
        // always remove successfully all elements
        return wasm::Literal(res);
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

      /// ext_storage_start_transaction_version_1
      if (import->base == ext_storage_start_transaction_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        host_api_->ext_storage_start_transaction_version_1();
        return wasm::Literal();
      }
      /// ext_storage_commit_transaction_version_1
      if (import->base == ext_storage_commit_transaction_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        host_api_->ext_storage_commit_transaction_version_1();
        return wasm::Literal();
      }
      /// ext_storage_rollback_transaction_version_1
      if (import->base == ext_storage_rollback_transaction_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        host_api_->ext_storage_rollback_transaction_version_1();
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
        return wasm::Literal(res);
      }

      // ------------------------- Offchain extension --------------------------

      /// ext_offchain_is_validator_version_1
      if (import->base == ext_offchain_is_validator_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_offchain_is_validator_version_1();
        return wasm::Literal(res);
      }

      /// ext_offchain_submit_transaction_version_1
      if (import->base == ext_offchain_submit_transaction_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_offchain_submit_transaction_version_1(
            arguments.at(0).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_network_state_version_1
      if (import->base == ext_offchain_network_state_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_offchain_network_state_version_1();
        return wasm::Literal(res);
      }

      /// ext_offchain_timestamp_version_1
      if (import->base == ext_offchain_timestamp_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_offchain_timestamp_version_1();
        return wasm::Literal(res);
      }

      /// ext_offchain_sleep_until_version_1
      if (import->base == ext_offchain_sleep_until_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_offchain_sleep_until_version_1(arguments.at(0).geti64());
        return wasm::Literal();
      }

      /// ext_offchain_random_seed_version_1
      if (import->base == ext_offchain_random_seed_version_1) {
        checkArguments(import->base.c_str(), 0, arguments.size());
        auto res = host_api_->ext_offchain_random_seed_version_1();
        return wasm::Literal(res);
      }

      /// ext_offchain_local_storage_set_version_1
      if (import->base == ext_offchain_local_storage_set_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        host_api_->ext_offchain_local_storage_set_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti64(),
            arguments.at(2).geti64());
        return wasm::Literal();
      }

      /// ext_offchain_local_storage_clear_version_1
      if (import->base == ext_offchain_local_storage_clear_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_offchain_local_storage_clear_version_1(
            arguments.at(0).geti32(), arguments.at(1).geti64());
        return wasm::Literal();
      }

      /// ext_offchain_local_storage_compare_and_set_version_1
      if (import->base
          == ext_offchain_local_storage_compare_and_set_version_1) {
        checkArguments(import->base.c_str(), 4, arguments.size());
        auto res =
            host_api_->ext_offchain_local_storage_compare_and_set_version_1(
                arguments.at(0).geti32(),
                arguments.at(1).geti64(),
                arguments.at(2).geti64(),
                arguments.at(3).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_local_storage_get_version_1
      if (import->base == ext_offchain_local_storage_get_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_offchain_local_storage_get_version_1(
            arguments.at(0).geti32(), arguments.at(1).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_http_request_start_version_1
      if (import->base == ext_offchain_http_request_start_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_offchain_http_request_start_version_1(
            arguments.at(0).geti64(),
            arguments.at(1).geti64(),
            arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_http_request_add_header_version_1
      if (import->base == ext_offchain_http_request_add_header_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_offchain_http_request_add_header_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti64(),
            arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_http_request_write_body_version_1
      if (import->base == ext_offchain_http_request_write_body_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_offchain_http_request_write_body_version_1(
            arguments.at(0).geti32(),
            arguments.at(1).geti64(),
            arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_http_response_wait_version_1
      if (import->base == ext_offchain_http_response_wait_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        auto res = host_api_->ext_offchain_http_response_wait_version_1(
            arguments.at(0).geti64(), arguments.at(1).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_http_response_headers_version_1
      if (import->base == ext_offchain_http_response_headers_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        auto res = host_api_->ext_offchain_http_response_headers_version_1(
            arguments.at(0).geti32());
        return wasm::Literal(res);
      }

      /// ext_offchain_http_response_read_body_version_1
      if (import->base == ext_offchain_http_response_read_body_version_1) {
        checkArguments(import->base.c_str(), 3, arguments.size());
        auto res = host_api_->ext_offchain_http_response_read_body_version_1(
            arguments.at(0).geti64(),
            arguments.at(1).geti64(),
            arguments.at(2).geti64());
        return wasm::Literal(res);
      }

      /// ext_offchain_set_authorized_nodes_version_1
      if (import->base == ext_offchain_set_authorized_nodes_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_offchain_set_authorized_nodes_version_1(
            arguments.at(0).geti64(), arguments.at(1).geti32());
        return wasm::Literal();
      }

      /// ext_offchain_index_set_version_1
      if (import->base == ext_offchain_index_set_version_1) {
        checkArguments(import->base.c_str(), 2, arguments.size());
        host_api_->ext_offchain_index_set_version_1(arguments.at(0).geti64(),
                                                    arguments.at(1).geti64());
        return wasm::Literal();
      }

      /// ext_offchain_index_clear_version_1
      if (import->base == ext_offchain_index_clear_version_1) {
        checkArguments(import->base.c_str(), 1, arguments.size());
        host_api_->ext_offchain_index_clear_version_1(arguments.at(0).geti64());
        return wasm::Literal();
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
      throw std::runtime_error(
          "Invocation of a Host API method with wrong number of arguments");
    }
  }

}  // namespace kagome::runtime::binaryen
