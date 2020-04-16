/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP

#include "authorship/block_builder_factory.hpp"

#include "blockchain/block_header_repository.hpp"
#include "common/logger.hpp"

namespace kagome::authorship {

  class BlockBuilderFactoryImpl : public BlockBuilderFactory {
   public:
    ~BlockBuilderFactoryImpl() override = default;

    BlockBuilderFactoryImpl(
        std::shared_ptr<runtime::Core> r_core,
        std::shared_ptr<runtime::BlockBuilder> r_block_builder,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_backend);

    outcome::result<std::unique_ptr<BlockBuilder>> create(
        const kagome::primitives::BlockId &parent_id,
        primitives::Digest inherent_digest) const override;

   private:
    std::shared_ptr<runtime::Core> r_core_;
    std::shared_ptr<runtime::BlockBuilder> r_block_builder_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_backend_;
    common::Logger logger_;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP
