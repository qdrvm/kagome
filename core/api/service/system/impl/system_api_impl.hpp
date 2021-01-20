/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEMAPIIMPL
#define KAGOME_API_SYSTEMAPIIMPL

#include "api/service/system/system_api.hpp"

#include "runtime/system.hpp"

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::api {

  class SystemApiImpl final : public SystemApi {
   public:
    SystemApiImpl(
        std::shared_ptr<application::ChainSpec> config,
        std::shared_ptr<consensus::Babe> babe,
        std::shared_ptr<network::Gossiper> gossiper,
        std::shared_ptr<runtime::System> system_api,
        std::shared_ptr<transaction_pool::TransactionPool> transaction_pool);

    std::shared_ptr<application::ChainSpec> getConfig() const override;

    std::shared_ptr<consensus::Babe> getBabe() const override;

    std::shared_ptr<network::Gossiper> getGossiper() const override;

    outcome::result<primitives::AccountNonce> getNonceFor(
        std::string_view account_address) const override;

   private:
    primitives::AccountNonce adjustNonce(
        const primitives::AccountId &account_id,
        primitives::AccountNonce current_nonce) const;

    std::shared_ptr<application::ChainSpec> config_;
    std::shared_ptr<consensus::Babe> babe_;
    std::shared_ptr<network::Gossiper> gossiper_;
    std::shared_ptr<runtime::System> system_api_;
    std::shared_ptr<transaction_pool::TransactionPool> transaction_pool_;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SYSTEMAPIIMPL
