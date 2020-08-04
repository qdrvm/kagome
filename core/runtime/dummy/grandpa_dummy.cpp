/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/dummy/grandpa_dummy.hpp"

#include <boost/system/error_code.hpp>

namespace kagome::runtime::dummy {

  GrandpaDummy::GrandpaDummy(
      std::shared_ptr<application::KeyStorage> key_storage)
      : key_storage_{std::move(key_storage)} {
    BOOST_ASSERT(key_storage_ != nullptr);
  }

  outcome::result<boost::optional<primitives::ScheduledChange>>
  GrandpaDummy::pending_change(const Grandpa::Digest &digest) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }
  outcome::result<boost::optional<primitives::ForcedChange>>
  GrandpaDummy::forced_change(const Grandpa::Digest &digest) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }
  outcome::result<primitives::AuthorityList> GrandpaDummy::authorities(
      const primitives::BlockId &block_id) {
    return primitives::AuthorityList{
        {{key_storage_->getLocalEd25519Keypair().public_key}, 1}};
  }
}  // namespace kagome::runtime::dummy
