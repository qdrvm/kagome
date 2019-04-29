#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(address_repository)
add_subdirectory(key_repository)
add_subdirectory(protocol_repository)


add_library(address_repository
    address_repository.cpp
    )
target_link_libraries(address_repository
    Boost::boost
    )


add_library(libp2p_peer_errors
    errors.cpp
    )
target_link_libraries(libp2p_peer_errors
    outcome
    )
