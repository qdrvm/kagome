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

#ifndef KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP

#include "authorship/block_builder_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::authorship {

  class BlockBuilderFactoryMock : public BlockBuilderFactory {
   public:
    // Dirty hack from https://stackoverflow.com/a/11548191 to overcome issue
    // with returning unique_ptr from gmock
    outcome::result<std::unique_ptr<BlockBuilder>> create(
        const primitives::BlockId &parent_id,
        primitives::Digest inherent_digest) const override {
      return std::unique_ptr<BlockBuilder>(
          createProxy(parent_id, std::move(inherent_digest)));
    }

    MOCK_CONST_METHOD2(createProxy,
                       BlockBuilder *(const primitives::BlockId &,
                                      primitives::Digest));
  };

}  // namespace kagome::authorship

#endif  // KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP
