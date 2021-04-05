#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_announce_protocol
    block_announce_protocol.cpp
    )
target_link_libraries(block_announce_protocol
    logger
    )

add_library(sync_protocol
    sync_protocol.cpp
    )
target_link_libraries(sync_protocol
    logger
    )
