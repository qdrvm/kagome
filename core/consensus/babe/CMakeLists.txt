#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(babe
    impl/babe_digests_util.cpp
    impl/babe.cpp
    impl/threshold_util.cpp
    impl/babe_lottery_impl.cpp
    impl/babe_block_validator_impl.cpp
    impl/babe_config_repository_impl.cpp
)
target_link_libraries(babe
    consensus_common
)
kagome_install(babe)
