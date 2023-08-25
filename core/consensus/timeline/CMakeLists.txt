#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(timeline
    impl/timeline_impl.cpp
    impl/consistency_keeper_impl.cpp
    impl/block_appender_base.cpp
    impl/block_executor_impl.cpp
    impl/block_header_appender_impl.cpp
    )
target_link_libraries(timeline
    logger
    )
kagome_install(timeline)
