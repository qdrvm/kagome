#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(multi)
add_subdirectory(muxer)
add_subdirectory(peer)
add_subdirectory(transport)
add_subdirectory(crypto)
#add_subdirectory(protocol_muxer)
add_subdirectory(event)
add_subdirectory(network)
add_subdirectory(security)
add_subdirectory(connection)

addtest(host_builder_test
    host_builder_test.cpp
    )
target_link_libraries(host_builder_test
    libp2p
    )
