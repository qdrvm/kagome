/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_GRANDPAAPI
#define KAGOME_RUNTIME_IMPL_GRANDPAAPI

#include "runtime/runtime_api/grandpa_api.hpp"

#include "blockchain/block_header_repository.hpp"

namespace kagome::runtime {

  class Executor;

  class GrandpaApiImpl final : public GrandpaApi {
   public:
    GrandpaApiImpl(
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<Executor> executor);

    outcome::result<std::optional<ScheduledChange>> pending_change(
        primitives::BlockHash const &block, const Digest &digest) override;

    outcome::result<std::optional<ForcedChange>> forced_change(
        primitives::BlockHash const &block, const Digest &digest) override;

    outcome::result<AuthorityList> authorities(
        const primitives::BlockId &block_id) override;

    outcome::result<primitives::AuthoritySetId> current_set_id(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_GRANDPAAPI
