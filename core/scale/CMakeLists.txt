#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(scale_libp2p_types
    libp2p_types.cpp
    )

target_link_libraries(scale_libp2p_types
    scale::scale
    p2p::p2p
    )
