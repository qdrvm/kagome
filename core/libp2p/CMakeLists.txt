#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(multi)
add_subdirectory(transport)
add_subdirectory(crypto)
add_subdirectory(peer)
add_subdirectory(muxer)
add_subdirectory(protocol_muxer)
add_subdirectory(network)

add_library(libp2p
    config.hpp
    host.cpp
    host.hpp
    host_builder.cpp
    host_builder.hpp
    )
target_link_libraries(libp2p
    outcome
    )
