/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CRYPTO_EXTENSION_HPP
#define KAGOME_CRYPTO_EXTENSION_HPP

#include <cstdint>

#include "common/logger.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::crypto {
  class SR25519Provider;
  class ED25519Provider;
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::extensions {
  /**
   * Implements extension functions related to cryptography
   */
  class CryptoExtension {
   public:
    explicit CryptoExtension(
        std::shared_ptr<runtime::WasmMemory> memory,
        std::shared_ptr<crypto::SR25519Provider> sr25519_provider,
        std::shared_ptr<crypto::ED25519Provider> ed25519_provider,
        std::shared_ptr<crypto::Hasher> hasher);

    /**
     * @see Extension::ext_blake2_256
     */
    void ext_blake2_256(runtime::WasmPointer data,
                        runtime::SizeType len,
                        runtime::WasmPointer out_ptr);

    /**
     * @see Extension::ext_keccak_256
     */
    void ext_keccak_256(runtime::WasmPointer data,
                        runtime::SizeType len,
                        runtime::WasmPointer out_ptr);

    /**
     * @see Extension::ext_ed25519_verify
     */
    runtime::SizeType ext_ed25519_verify(runtime::WasmPointer msg_data,
                                         runtime::SizeType msg_len,
                                         runtime::WasmPointer sig_data,
                                         runtime::WasmPointer pubkey_data);
    /**
     * @see Extension::ext_sr25519_verify
     */
    runtime::SizeType ext_sr25519_verify(runtime::WasmPointer msg_data,
                                         runtime::SizeType msg_len,
                                         runtime::WasmPointer sig_data,
                                         runtime::WasmPointer pubkey_data);

    /**
     * @see Extension::ext_twox_64
     */
    void ext_twox_64(runtime::WasmPointer data,
                      runtime::SizeType len,
                      runtime::WasmPointer out);
    
    /**
     * @see Extension::ext_twox_128
     */
    void ext_twox_128(runtime::WasmPointer data,
                      runtime::SizeType len,
                      runtime::WasmPointer out);

    /**
     * @see Extension::ext_twox_256
     */
    void ext_twox_256(runtime::WasmPointer data,
                      runtime::SizeType len,
                      runtime::WasmPointer out);

   private:
    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<crypto::SR25519Provider> sr25519_provider_;
    std::shared_ptr<crypto::ED25519Provider> ed25519_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    common::Logger logger_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_CRYPTO_EXTENSION_HPP
