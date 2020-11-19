/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_DUMMY_GRANDPAAPIDUMMY
#define KAGOME_CORE_RUNTIME_DUMMY_GRANDPAAPIDUMMY

#include "runtime/grandpa_api.hpp"
#include "crypto/crypto_store.hpp"

namespace kagome::runtime::dummy {

  /**
   * Dummy implementation of the grandpa api. Should not be used in production.
   * Instead of using runtime to get authorities, just returns current authority
   */
  class GrandpaApiDummy : public GrandpaApi {
   public:
    ~GrandpaApiDummy() override = default;

    explicit GrandpaApiDummy(
        std::shared_ptr<crypto::CryptoStore> crypto_store);

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<AuthorityList> authorities(
        const primitives::BlockId &block_id) override;

   private:
    std::shared_ptr<crypto::CryptoStore> crypto_store_;
  };
}  // namespace kagome::runtime::dummy

#endif  // KAGOME_CORE_RUNTIME_DUMMY_GRANDPAAPIDUMMY
