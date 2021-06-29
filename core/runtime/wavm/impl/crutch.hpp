/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stack>

#include <WAVM/IR/Module.h>
#include <WAVM/IR/Types.h>
#include <WAVM/IR/Value.h>
#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Runtime.h>

#include "host_api/host_api.hpp"
#include "log/logger.hpp"

namespace kagome::runtime::wavm {

  class IntrinsicResolver;

  void pushHostApi(std::shared_ptr<host_api::HostApi>);
  void popHostApi();
  std::shared_ptr<host_api::HostApi> peekHostApi();

  extern log::Logger logger;

  extern const WAVM::IR::MemoryType kIntrinsicMemoryType;
  constexpr std::string_view kIntrinsicMemoryName = "Host memory";

  WAVM::Intrinsics::Module *getIntrinsicModule_env();

  void registerHostApiMethods(IntrinsicResolver& resolver, const host_api::HostApi& host_api);

#define DECLARE_HOST_INTRINSIC(Ret, name, ...)                    \
  Ret name(WAVM::Runtime::ContextRuntimeData *contextRuntimeData, \
           ##__VA_ARGS__);                                        \
  extern WAVM::Intrinsics::Function name##Intrinsic;

  DECLARE_HOST_INTRINSIC(
      void, ext_logging_log_version_1, WAVM::I32, WAVM::I64, WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32, ext_hashing_twox_128_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32, ext_hashing_twox_64_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(void, ext_storage_set_version_1, WAVM::I64, WAVM::I64);
  DECLARE_HOST_INTRINSIC(void, ext_storage_clear_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_hashing_blake2_128_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(void, ext_storage_clear_prefix_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I64, ext_storage_get_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(void, ext_misc_print_utf8_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32, ext_offchain_random_seed_version_1);
  DECLARE_HOST_INTRINSIC(void, ext_misc_print_hex_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(void, ext_crypto_start_batch_verify_version_1);
  DECLARE_HOST_INTRINSIC(WAVM::I32, ext_crypto_finish_batch_verify_version_1);
  DECLARE_HOST_INTRINSIC(WAVM::I32, ext_offchain_is_validator_version_1);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_offchain_local_storage_get_version_1,
                         WAVM::I32,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_offchain_local_storage_compare_and_set_version_1,
                         WAVM::I32,
                         WAVM::I64,
                         WAVM::I64,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_hashing_blake2_256_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_hashing_keccak_256_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_crypto_ed25519_verify_version_1,
                         WAVM::I32,
                         WAVM::I64,
                         WAVM::I32);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_misc_runtime_version_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(void,
                         ext_storage_append_version_1,
                         WAVM::I64,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I64, ext_storage_next_key_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(void, ext_misc_print_num_version_1, WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_crypto_sr25519_verify_version_2,
                         WAVM::I32,
                         WAVM::I64,
                         WAVM::I32);
  DECLARE_HOST_INTRINSIC(void,
                         ext_offchain_local_storage_set_version_1,
                         WAVM::I32,
                         WAVM::I64,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I64, ext_storage_root_version_1);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_storage_changes_root_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_trie_blake2_256_ordered_root_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_crypto_ed25519_generate_version_1,
                         WAVM::I32,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_crypto_secp256k1_ecdsa_recover_version_1,
                         WAVM::I32,
                         WAVM::I32);
  DECLARE_HOST_INTRINSIC(
      WAVM::I64,
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_1,
      WAVM::I32,
      WAVM::I32);
  DECLARE_HOST_INTRINSIC(WAVM::I32,
                         ext_crypto_sr25519_generate_version_1,
                         WAVM::I32,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_crypto_sr25519_public_keys_version_1,
                         WAVM::I32);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_crypto_sr25519_sign_version_1,
                         WAVM::I32,
                         WAVM::I32,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(WAVM::I64, ext_offchain_network_state_version_1);
  DECLARE_HOST_INTRINSIC(WAVM::I64,
                         ext_offchain_submit_transaction_version_1,
                         WAVM::I64);
  DECLARE_HOST_INTRINSIC(
      WAVM::I64, ext_storage_read_version_1, WAVM::I64, WAVM::I64, WAVM::I32);
  DECLARE_HOST_INTRINSIC(WAVM::I32, ext_allocator_malloc_version_1, WAVM::I32);
  DECLARE_HOST_INTRINSIC(void, ext_allocator_free_version_1, WAVM::I32);

}  // namespace kagome::runtime::wavm
