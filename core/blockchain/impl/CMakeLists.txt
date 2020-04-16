#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
#
# Kagome is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kagome is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

add_library(blockchain_common
    types.cpp
    common.hpp
    storage_util.cpp
    storage_util.hpp
    )
target_link_libraries(blockchain_common
    blob
    Boost::boost
    buffer
    database_error
    in_memory_storage
    polkadot_trie_db
    trie_db_backend
    primitives
    )

add_library(block_tree
    block_tree_impl.cpp
    block_tree_impl.hpp
    )
target_link_libraries(block_tree
    block_tree_error
    blockchain_common
    leveldb
    hasher
    )
