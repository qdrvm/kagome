/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_AUTHOR_IMPL_AUTHOR_API_IMPL_HPP
#define KAGOME_CORE_API_SERVICE_AUTHOR_IMPL_AUTHOR_API_IMPL_HPP

/**
 * ExtrinsicSubmissionApi based on auth api implemented in substrate here
 * https://github.com/paritytech/substrate/blob/e8739300ae3f7f2e7b72f64668573275f2806ea5/core/rpc/src/author/mod.rs#L50-L49
 */

#include "api/service/author/author_api.hpp"

#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>

#include <boost/di/extension/injections/lazy.hpp>

#include "common/blob.hpp"
#include "log/logger.hpp"
#include "primitives/author_api_primitives.hpp"
#include "primitives/transaction.hpp"

namespace kagome::api {
  class ApiService;
}
namespace kagome::blockchain {
  class BlockTree;
}
namespace kagome::crypto {
  class CryptoStore;
  class Hasher;
  class KeyFileStorage;
  class SessionKeys;
}  // namespace kagome::crypto
namespace kagome::network {
  class TransactionsTransmitter;
}
namespace kagome::primitives {
  struct Extrinsic;
}
namespace kagome::runtime {
  class SessionKeysApi;
}
namespace kagome::transaction_pool {
  class TransactionPool;
}
namespace kagome::subscription {
  template <typename Event, typename Receiver, typename... Arguments>
  class Subscriber;
}

namespace kagome::api {

  template <typename T>
  using lazy = boost::di::extension::lazy<T>;

  class AuthorApiImpl : public AuthorApi {
    template <class T>
    using sptr = std::shared_ptr<T>;
    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    /**
     * @constructor
     * @param api ttq instance shared ptr
     * @param pool transaction pool instance shared ptr
     * @param hasher hasher instance shared ptr
     * @param block_tree block tree instance shared ptr
     */
    AuthorApiImpl(sptr<runtime::SessionKeysApi> key_api,
                  sptr<transaction_pool::TransactionPool> pool,
                  sptr<crypto::CryptoStore> store,
                  sptr<crypto::SessionKeys> keys,
                  sptr<crypto::KeyFileStorage> key_store,
                  lazy<sptr<blockchain::BlockTree>> block_tree,
                  lazy<std::shared_ptr<api::ApiService>> api_service);

    ~AuthorApiImpl() override = default;

    outcome::result<common::Hash256> submitExtrinsic(
        TransactionSource source,
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<void> insertKey(
        crypto::KeyTypeId key_type,
        const gsl::span<const uint8_t> &seed,
        const gsl::span<const uint8_t> &public_key) override;

    outcome::result<common::Buffer> rotateKeys() override;

    outcome::result<bool> hasSessionKeys(
        const gsl::span<const uint8_t> &keys) override;

    outcome::result<bool> hasKey(const gsl::span<const uint8_t> &public_key,
                                 crypto::KeyTypeId key_type) override;

    outcome::result<std::vector<primitives::Extrinsic>> pendingExtrinsics()
        override;

    outcome::result<std::vector<primitives::Extrinsic>> removeExtrinsic(
        const std::vector<primitives::ExtrinsicKey> &keys) override;

    outcome::result<SubscriptionId> submitAndWatchExtrinsic(
        Extrinsic extrinsic) override;

    outcome::result<bool> unwatchExtrinsic(
        SubscriptionId subscription_id) override;

   private:
    sptr<runtime::SessionKeysApi> keys_api_;
    sptr<transaction_pool::TransactionPool> pool_;
    sptr<crypto::CryptoStore> store_;
    sptr<crypto::SessionKeys> keys_;
    sptr<crypto::KeyFileStorage> key_store_;
    lazy<std::shared_ptr<api::ApiService>> api_service_;
    lazy<sptr<blockchain::BlockTree>> block_tree_;

    log::Logger logger_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_AUTHOR_IMPL_AUTHOR_API_IMPL_HPP
