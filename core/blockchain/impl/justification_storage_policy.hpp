/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_JUSTIFICATION_STORAGE_POLICY_HPP
#define KAGOME_JUSTIFICATION_STORAGE_POLICY_HPP

#include "primitives/common.hpp"

namespace kagome::authority {
  class AuthorityManager;
}

namespace kagome::blockchain {

  class JustificationStoragePolicy {
   public:
    virtual ~JustificationStoragePolicy() = default;

    virtual bool shouldStore(primitives::BlockInfo block) const = 0;
  };

  class JustificationStoragePolicyImpl final
      : public JustificationStoragePolicy {
   public:
    virtual bool shouldStore(primitives::BlockInfo block) const override;

    virtual void initAuthorityManager(
        std::shared_ptr<const authority::AuthorityManager>
            auth_manager);

   private:
    std::shared_ptr<const authority::AuthorityManager> auth_manager_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_JUSTIFICATION_STORAGE_POLICY_HPP
