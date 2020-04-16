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

add_library(babe_lottery
    babe_lottery_impl.cpp
    babe_lottery_impl.hpp
    )
target_link_libraries(babe_lottery
    hasher
    vrf_provider
    logger
    )

add_library(threshold_util
    threshold_util.cpp
    )
target_link_libraries(threshold_util
    primitives
    )

add_library(babe_digests_util
    babe_digests_util.cpp
    )
target_link_libraries(babe_digests_util
    primitives
    scale
    )

add_library(babe
    babe_impl.cpp
    babe_impl.hpp
    )
target_link_libraries(babe
    babe_error
    Boost::boost
    sr25519::sr25519
    buffer
    logger
    scale
    blob
    outcome
    p2p::p2p_peer_id
    babe_digests_util
    threshold_util
    block_tree_error
    )

add_library(babe_synchronizer
    babe_synchronizer_impl.cpp
    )
target_link_libraries(babe_synchronizer
    logger
    primitives
    )
