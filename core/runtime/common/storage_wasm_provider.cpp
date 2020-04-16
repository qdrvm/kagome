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

#include "runtime/common/storage_wasm_provider.hpp"

namespace kagome::runtime {

  StorageWasmProvider::StorageWasmProvider(
      std::shared_ptr<storage::trie::TrieDb> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_ != nullptr);

    last_state_root_ = storage_->getRootHash();
    auto state_code_res = storage_->get(kRuntimeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    state_code_ = state_code_res.value();
  }

  const common::Buffer &StorageWasmProvider::getStateCode() const {
    auto current_state_root = storage_->getRootHash();
    if (last_state_root_ == current_state_root) {
      return state_code_;
    }
    last_state_root_ = current_state_root;

    auto state_code_res = storage_->get(kRuntimeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    state_code_ = state_code_res.value();
    return state_code_;
  }

}  // namespace kagome::runtime
