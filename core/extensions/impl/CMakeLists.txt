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

add_library(crypto_extension
    crypto_extension.cpp
    )
target_link_libraries(crypto_extension
    hasher
    p2p::p2p_random_generator
    sr25519_provider
    ed25519_provider
    logger
    )
kagome_install(crypto_extension)

add_library(io_extension
    io_extension.cpp
    )
target_link_libraries(io_extension
    hexutil
    logger
    buffer
    )
kagome_install(io_extension)

add_library(memory_extension
    memory_extension.cpp
    )
target_link_libraries(memory_extension
    logger
    buffer
)
kagome_install(memory_extension)

add_library(misc_extension
    misc_extension.cpp
    )
kagome_install(misc_extension)

add_library(storage_extension
    storage_extension.cpp
    )
target_link_libraries(storage_extension
    buffer
    blob
    logger
    ordered_trie_hash
    )
kagome_install(storage_extension)
