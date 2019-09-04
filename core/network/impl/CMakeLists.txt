#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(stream_manager_libp2p
    stream_manager_libp2p.cpp
    stream_manager_libp2p.hpp
    )
target_link_libraries(stream_manager_libp2p
    outcome
    )

add_library(gossiper_broadcast
    gossiper_broadcast.cpp
    gossiper_broadcast.hpp
    )
target_link_libraries(gossiper_broadcast
    stream_manager_libp2p
    scale_message_read_writer
    )
