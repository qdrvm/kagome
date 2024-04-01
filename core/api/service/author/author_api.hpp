/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/buffer_view.hpp"
#include "crypto/common.hpp"
#include "crypto/key_store/key_type.hpp"
#include "primitives/author_api_primitives.hpp"
#include "primitives/transaction_validity.hpp"

namespace kagome::api {
  class ApiService;
}

namespace kagome::primitives {
  struct Extrinsic;
}

namespace kagome::api {
  class AuthorApi {
   protected:
    using Hash256 = common::Hash256;
    using Buffer = common::Buffer;
    using BufferView = common::BufferView;
    using Extrinsic = primitives::Extrinsic;
    using SubscriptionId = primitives::SubscriptionId;
    using ExtrinsicKey = primitives::ExtrinsicKey;
    using TransactionSource = primitives::TransactionSource;

   public:
    virtual ~AuthorApi() = default;

    /**
     * @brief validates and sends extrinsic to transaction pool
     * @param source how extrinsic was received (for example external or
     * submitted through offchain worker)
     * @param extrinsic set of bytes representing either transaction or inherent
     * @return hash of successfully validated extrinsic
     * or error if state is invalid or unknown
     */
    virtual outcome::result<common::Hash256> submitExtrinsic(
        TransactionSource source, const Extrinsic &extrinsic) = 0;

    /**
     * @brief insert an anonimous key pair into the keystore
     * @param key_type Key type
     * @param seed The seed (suri) in binary
     * @param public_key The public key in binary
     */
    virtual outcome::result<void> insertKey(crypto::KeyType key_type,
                                            crypto::SecureBuffer<> seed,
                                            const BufferView &public_key) = 0;

    /**
     * @brief Generate new session keys and
     * returns the corresponding public keys
     * @return The SCALE encoded, concatenated keys
     */
    virtual outcome::result<common::Buffer> rotateKeys() = 0;

    /**
     * @brief checks if the keystore has private keys for the given session
     * public keys
     * @param keys SCALE encoded concatenated keys
     * @return returns true if all private keys could be found, false if
     * otherwise
     */
    virtual outcome::result<bool> hasSessionKeys(const BufferView &keys) = 0;

    /**
     * @brief checks if the keystore has private keys for the given public
     * key and key type
     * @param public_key The public key in binary
     * @param key_type The key type
     */
    virtual outcome::result<bool> hasKey(const BufferView &public_key,
                                         crypto::KeyType key_type) = 0;

    /**
     * @return collection of pending extrinsics
     */
    virtual outcome::result<std::vector<Extrinsic>> pendingExtrinsics() = 0;

    /**
     * Remove given extrinsic from the pool and temporarily ban it to prevent
     * reimporting.
     */
    virtual outcome::result<std::vector<Extrinsic>> removeExtrinsic(
        const std::vector<ExtrinsicKey> &keys) = 0;

    /**
     * Submit an extrinsic and watch.
     */
    virtual outcome::result<SubscriptionId> submitAndWatchExtrinsic(
        Extrinsic extrinsic) = 0;

    /**
     * Unsubscribe from extrinsic watching.
     * @see AuthorApi::submitAndWatchExtrinsic
     * @return true if the subscriber was unsubscribed, false if there was no
     * subscriber.
     */
    virtual outcome::result<bool> unwatchExtrinsic(SubscriptionId sub_id) = 0;
  };
}  // namespace kagome::api
