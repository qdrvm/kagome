#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(sassafras
    impl/sassafras_digests_util.cpp
    impl/sassafras.cpp
    impl/threshold_util.cpp
    impl/sassafras_lottery_impl.cpp
    impl/sassafras_block_validator_impl.cpp
    impl/sassafras_config_repository_impl.cpp
    impl/sassafras_error.cpp
#    impl/sassafras_vrf.cpp
)
target_link_libraries(sassafras
    consensus_common
)
kagome_install(sassafras)
kagome_clear_objects(sassafras)

