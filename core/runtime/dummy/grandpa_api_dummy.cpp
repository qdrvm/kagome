/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/dummy/grandpa_api_dummy.hpp"

#include <boost/system/error_code.hpp>

namespace kagome::runtime::dummy {

  GrandpaApiDummy::GrandpaApiDummy(
      std::shared_ptr<application::AKeyStorage> key_storage)
      : key_storage_{std::move(key_storage)} {
    BOOST_ASSERT(key_storage_ != nullptr);
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
    return primitives::AuthorityList{
        {{key_storage_->getLocalEd25519Keypair().public_key}, 1}};
  }
}  // namespace kagome::runtime::dummy
