/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_GRANDPA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_GRANDPA_IMPL_HPP

#include "runtime/grandpa_api.hpp"

#include "blockchain/block_header_repository.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"

namespace kagome::runtime::binaryen {

  class GrandpaApiImpl : public RuntimeApi, public GrandpaApi {
   public:
    explicit GrandpaApiImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory,
        const std::shared_ptr<blockchain::BlockHeaderRepository> &header_repo);

    ~GrandpaApiImpl() override = default;

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override;

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override;

    outcome::result<primitives::AuthorityList> authorities(
        const primitives::BlockId &block_id) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_GRANDPA_IMPL_HPP
