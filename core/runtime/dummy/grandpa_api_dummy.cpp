/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/dummy/grandpa_api_dummy.hpp"

#include <boost/system/error_code.hpp>

namespace kagome::runtime::dummy {

  GrandpaApiDummy::GrandpaApiDummy(
      std::shared_ptr<crypto::CryptoStore> crypto_store)
      : crypto_store_{std::move(crypto_store)} {
    BOOST_ASSERT(crypto_store_ != nullptr);
  }

  outcome::result<boost::optional<primitives::ScheduledChange>>
  GrandpaApiDummy::pending_change(const GrandpaApi::Digest &digest) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }
  outcome::result<boost::optional<primitives::ForcedChange>>
  GrandpaApiDummy::forced_change(const GrandpaApi::Digest &digest) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }
  outcome::result<primitives::AuthorityList> GrandpaApiDummy::authorities(
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(keys, crypto_store_->getEd25519PublicKeys(crypto::KEY_TYPE_GRAN));
    return primitives::AuthorityList{
        {{keys.at(0)}, 1}};
  }
}  // namespace kagome::runtime::dummy
