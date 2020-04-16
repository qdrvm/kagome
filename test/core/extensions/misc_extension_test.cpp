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

#include <gtest/gtest.h>
#include "extensions/impl/misc_extension.hpp"

/**
 * @given a chain id
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST(MiscExt, Init) {
  kagome::extensions::MiscExtension m;
  ASSERT_EQ(m.ext_chain_id(), 42);

  kagome::extensions::MiscExtension m2(34);
  ASSERT_EQ(m2.ext_chain_id(), 34);
}
