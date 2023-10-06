/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_SESSIONKEYSAPI
#define KAGOME_RUNTIME_SESSIONKEYSAPI

#include <optional>

#include <outcome/outcome.hpp>

#include "crypto/crypto_store/key_type.hpp"
#include "primitives/common.hpp"
#include "primitives/metadata.hpp"

namespace kagome::runtime {

  class SessionKeysApi {
   public:
    virtual ~SessionKeysApi() = default;

    /**
     * @brief Generate a set of session keys with optionally using the given
     * seed. The keys should be stored within the keystore exposed via runtime
     * externalities.
     * @param seed - optional seed, which needs to be a valid `utf8` string.
     * @return the concatenated SCALE encoded public keys.
     */
    virtual outcome::result<common::Buffer> generate_session_keys(
        const primitives::BlockHash &block_hash,
        std::optional<common::Buffer> seed) = 0;

    /**
     * @brief Decode the given public session keys.
     * @return the list of public raw public keys + key type.
     */
    virtual outcome::result<
        std::vector<std::pair<crypto::KeyType, common::Buffer>>>
    decode_session_keys(const primitives::BlockHash &block_hash,
                        common::BufferView encoded) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_SESSIONKEYSAPI
