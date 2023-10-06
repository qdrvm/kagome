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
    impl/babe_config_repository_impl.cpp
    impl/babe_error.cpp
    )
target_link_libraries(babe
    logger
    metrics
    )
kagome_install(babe)
