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

#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  int BranchNode::getType() const {
    return static_cast<int>(value ? PolkadotNode::Type::BranchWithValue
                                  : PolkadotNode::Type::BranchEmptyValue);
  }

  uint16_t BranchNode::childrenBitmap() const {
    uint16_t bitmap = 0u;
    for (auto i = 0u; i < kMaxChildren; i++) {
      if (children.at(i)) {
        bitmap = bitmap | 1u << i;
      }
    }
    return bitmap;
  }

  uint8_t BranchNode::childrenNum() const {
    return std::count_if(children.begin(),
                         children.end(),
                         [](auto const &child) { return child; });
    ;
  }

  int LeafNode::getType() const {
    return static_cast<int>(PolkadotNode::Type::Leaf);
  }

}  // namespace kagome::storage::trie
