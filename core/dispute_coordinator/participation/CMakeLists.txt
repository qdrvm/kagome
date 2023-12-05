#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(dispute_participation
    impl/participation_impl.cpp
    impl/queues_impl.cpp
    )

target_link_libraries(dispute_participation
    network
    )
