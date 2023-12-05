#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(dispute_provisioner
    impl/request_votes.cpp
    impl/prioritized_selection.cpp
    impl/random_selection.cpp
    )

target_link_libraries(dispute_provisioner
    network
    )
