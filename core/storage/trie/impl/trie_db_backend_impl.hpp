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

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_HPP
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_HPP

#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "storage/trie/trie_db_backend.hpp"

namespace kagome::storage::trie {

  class TrieDbBackendImpl : public TrieDbBackend {
   public:
    TrieDbBackendImpl(std::shared_ptr<BufferStorage> storage,
                      common::Buffer node_prefix);

    ~TrieDbBackendImpl() override = default;

    std::unique_ptr<face::MapCursor<Buffer, Buffer>> cursor() override;
    std::unique_ptr<face::WriteBatch<Buffer, Buffer>> batch() override;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;

   private:
    common::Buffer prefixKey(const common::Buffer &key) const;

    std::shared_ptr<BufferStorage> storage_;
    common::Buffer node_prefix_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_HPP
