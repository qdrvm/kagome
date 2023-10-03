/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/system/system_api.hpp"

#include "runtime/runtime_api/account_nonce_api.hpp"

namespace kagome::transaction_pool {
  class TransactionPool;
}
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::api {

  class SystemApiImpl final : public SystemApi {
   public:
    SystemApiImpl(
        std::shared_ptr<application::ChainSpec> config,
        std::shared_ptr<consensus::Timeline> timeline,
        std::shared_ptr<network::PeerManager> peer_manager,
        std::shared_ptr<runtime::AccountNonceApi> account_nonce_api,
        std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
        std::shared_ptr<const blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::Hasher> hasher);

    std::shared_ptr<application::ChainSpec> getConfig() const override;

    std::shared_ptr<consensus::Timeline> getTimeline() const override;

    std::shared_ptr<network::PeerManager> getPeerManager() const override;

    /**
     * The nonce which should be used for the next extrinsic from \arg
     * account_adderss
     */
    outcome::result<primitives::AccountNonce> getNonceFor(
        std::string_view account_address) const override;

   private:
    // adjusts the provided nonce considering the pending transactions
    primitives::AccountNonce adjustNonce(
        const primitives::AccountId &account_id,
        primitives::AccountNonce current_nonce) const;

    std::shared_ptr<application::ChainSpec> config_;
    std::shared_ptr<consensus::Timeline> timeline_;
    std::shared_ptr<network::PeerManager> peer_manager_;
    std::shared_ptr<runtime::AccountNonceApi> account_nonce_api_;
    std::shared_ptr<transaction_pool::TransactionPool> transaction_pool_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::api
