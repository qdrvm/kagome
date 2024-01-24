#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(outcome INTERFACE)

get_target_property(P2P_INCLUDE_DIRS p2p::p2p INTERFACE_INCLUDE_DIRECTORIES)

target_link_libraries(outcome INTERFACE
    Boost::boost
    fmt::fmt
    )
target_include_directories(outcome INTERFACE ${P2P_INCLUDE_DIRS})
kagome_install(outcome)
